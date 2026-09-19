#include <cassert>
#include <cmath>
#include <limits>
#include <iostream>
#include "../../firmware/cores3_base/src/face/FaceRenderer.h"
#include "../../firmware/cores3_base/src/ghost/BehaviorEngine.h"
std::vector<DrawCall> M5Canvas::rects;
int M5Canvas::pushes = 0;
using namespace deskbot;
int main() {
  face::EyeShape target; target.width_scale = 1.2f; target.upper_lid = .5f;
  face::ShapeTransition a,b;
  a.update({}, {}, 1, 0); b.update({}, {}, 1, 0);
  a.update(target, {}, 1.1f, 120);
  for (uint32_t t=20;t<=120;t+=20) b.update(target, {}, 1.1f,t);
  assert(fabs(a.left().width_scale-b.left().width_scale)<1e-6);
  assert(a.left().width_scale>1 && a.left().width_scale<1.2f);
  assert(a.right().width_scale==1);
  face::ShapeTransition wrap;
  wrap.update({}, {}, 1, UINT32_MAX-59);
  wrap.update(target, {}, 1.1f, 60);
  assert(fabs(a.left().width_scale-wrap.left().width_scale)<1e-6);
  target.width_scale = std::numeric_limits<float>::infinity();
  target.height_scale = -100; target.radius_scale = NAN;
  auto bounded = face::boundedShape(target);
  assert(bounded.width_scale==1 && bounded.height_scale==.6f && bounded.radius_scale==1);

  M5GFX display; face::FaceRenderer renderer; renderer.begin(display);
  auto p=face::FaceRenderer::neutral(); renderer.render(p,0);
  // Existing neutral: large 88x61 eyes, centered at (92,112)/(228,112).
  assert(M5Canvas::rects.size()==2);
  auto l=M5Canvas::rects[0], r=M5Canvas::rects[1];
  assert(l.x==48 && l.y==82 && l.w==88 && l.h==61 && l.radius==17);
  assert(r.x==184 && r.y==82 && r.w==88 && r.h==61 && r.radius==17);
  p.left.upper_lid=.4f; p.right.lower_lid=.3f;
  renderer.render(p,1000);
  assert(M5Canvas::rects.size()==4);
  assert(M5Canvas::rects[1].color==TFT_BLACK && M5Canvas::rects[3].color==TFT_BLACK);
  p.left.openness=p.right.openness=0;
  renderer.render(p,1040);
  assert(M5Canvas::rects.size()==2 && M5Canvas::rects[0].h==5);
  // All allowed geometry extremes with maximum gaze and shake stay on screen.
  for(float width : {.65f,1.2f}) for(float height : {.6f,1.25f})
  for(float spacing : {.85f,1.12f}) for(float offset : {-1.f,1.f}) {
    face::FaceRenderer f; f.begin(display); p=face::FaceRenderer::neutral();
    p.left.width_scale=p.right.width_scale=width;
    p.left.height_scale=p.right.height_scale=height;
    p.left.radius_scale=p.right.radius_scale=2;
    p.left.openness=p.right.openness=1;
    p.spacing_scale=spacing; p.offset_x=p.offset_y=p.jitter_x=p.jitter_y=offset;
    f.render(p,0);
  }
  ghost::BehaviorEngine behavior; ghost::HeartContext context;
  behavior.begin(0); behavior.tick(1000,context);
  assert(behavior.microBehavior().left_shape.width_scale==1);
  for(auto type : {nerve::NeuronType::TOUCH,nerve::NeuronType::PICKED_UP,nerve::NeuronType::SHAKE}) {
    behavior.onNeuron(nerve::makeNeuron(type,nerve::NeuronSource::IMU,1000),context);
    behavior.tick(1040,context); auto active=behavior.microBehavior();
    if(type==nerve::NeuronType::TOUCH) assert(active.left_shape.lower_lid>0);
    if(type==nerve::NeuronType::PICKED_UP) assert(active.left_shape.height_scale>1);
    if(type==nerve::NeuronType::SHAKE) assert(active.jitter_x!=0 && active.left_shape.upper_lid>0);
    behavior.tick(1800,context); auto expired=behavior.microBehavior();
    assert(expired.jitter_x==0 && expired.jitter_y==0);
    assert(expired.left_shape.upper_lid==0 && expired.left_shape.lower_lid==0);
    assert(expired.right_shape.height_scale==1 && expired.eye_spacing_scale==1);
  }
  behavior.tick(4300,context);
  assert(behavior.microBehavior().eye_openness==0);

  // Heart-driven autonomous behavior: baseline curiosity chooses a look.
  ghost::BehaviorEngine autonomousBehavior;
  ghost::HeartContext curiousContext;
  autonomousBehavior.begin(0);
  autonomousBehavior.tick(14999,curiousContext);
  assert(autonomousBehavior.autonomousAction()==ghost::AutonomousAction::NONE);
  autonomousBehavior.tick(15000,curiousContext);
  assert(autonomousBehavior.autonomousAction()==ghost::AutonomousAction::CURIOUS_LOOK);
  autonomousBehavior.tick(15500,curiousContext);
  assert(std::fabs(autonomousBehavior.microBehavior().gaze_x) > 0.10f);

  // Vision arbitration uses Ghost handling-time clock, not source timestamp.
  // Simulate a camera event whose occurrence timestamp is older than delivery.
  // Vision temporarily overlays but does not cancel the autonomous decision.
  curiousContext.captured_ms = 15620;
  autonomousBehavior.onNeuron(
      nerve::makeNeuron(nerve::NeuronType::MOTION_DETECTED,
                        nerve::NeuronSource::CAMERA_M5,15510),
      curiousContext);
  assert(autonomousBehavior.autonomousAction()==ghost::AutonomousAction::CURIOUS_LOOK);
  assert(autonomousBehavior.autonomousPaused());
  assert(autonomousBehavior.lastAutonomousLifecycle()==ghost::AutonomousLifecycle::PAUSE);

  // Repeated Vision extends the same pause without consuming action lifetime.
  autonomousBehavior.tick(15700,curiousContext);
  curiousContext.captured_ms = 16020;
  autonomousBehavior.onNeuron(
      nerve::makeNeuron(nerve::NeuronType::MOTION_DETECTED,
                        nerve::NeuronSource::CAMERA_M5,15900),
      curiousContext);
  assert(autonomousBehavior.autonomousPaused());
  autonomousBehavior.tick(16300,curiousContext);
  assert(autonomousBehavior.autonomousPaused());

  // 600 ms after the latest Vision event, action resumes with its remaining time.
  autonomousBehavior.tick(16620,curiousContext);
  assert(!autonomousBehavior.autonomousPaused());
  assert(autonomousBehavior.lastAutonomousLifecycle()==ghost::AutonomousLifecycle::RESUME);
  assert(autonomousBehavior.autonomousAction()==ghost::AutonomousAction::CURIOUS_LOOK);
  autonomousBehavior.tick(17000,curiousContext);
  assert(autonomousBehavior.autonomousAction()==ghost::AutonomousAction::CURIOUS_LOOK);
  assert(std::fabs(autonomousBehavior.microBehavior().gaze_x) > 0.10f);

  // It completes only after the remaining autonomous lifetime is actually used.
  autonomousBehavior.tick(18020,curiousContext);
  assert(autonomousBehavior.autonomousAction()==ghost::AutonomousAction::NONE);
  assert(autonomousBehavior.lastAutonomousLifecycle()==ghost::AutonomousLifecycle::COMPLETE);

  // Start another action, then direct body interaction cancels it immediately.
  autonomousBehavior.tick(33000,curiousContext);
  assert(autonomousBehavior.autonomousAction()==ghost::AutonomousAction::CURIOUS_LOOK);
  curiousContext.captured_ms = 33120;
  autonomousBehavior.onNeuron(
      nerve::makeNeuron(nerve::NeuronType::TOUCH,nerve::NeuronSource::TOUCH,33100),
      curiousContext);
  assert(autonomousBehavior.autonomousAction()==ghost::AutonomousAction::NONE);
  assert(autonomousBehavior.lastAutonomousLifecycle()==ghost::AutonomousLifecycle::CANCEL);

  // High boredom selects a scan instead of a curiosity glance.
  ghost::BehaviorEngine boredBehavior;
  ghost::HeartContext boredContext;
  boredContext.state.boredom = 0.50f;
  boredContext.state.curiosity = 0.20f;
  boredBehavior.begin(0);
  boredBehavior.tick(15000,boredContext);
  assert(boredBehavior.autonomousAction()==ghost::AutonomousAction::BORED_SCAN);
  boredBehavior.tick(16000,boredContext);
  assert(std::fabs(boredBehavior.microBehavior().gaze_x) > 0.05f);
  std::cout << "PASS: neutral geometry, independent lids, blink, bounds, frame timing, clock wrap, transient expiry, autonomous Heart-driven behavior, Vision pause/resume arbitration\n";
}

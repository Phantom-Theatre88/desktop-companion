#include "../../firmware/cores3_base/src/body/BodyOutputComposer.h"
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
  // Immediate expression belongs to Reflex -> Body, not Behavior.
  ghost::BehaviorEngine behavior;
  ghost::HeartContext context;
  body::BodyOutputComposer composer;
  reflex::ReflexLayer reflex;
  reflex.setIntentHandler([](const reflex::ReflexIntent& intent, void* ctx) {
    static_cast<body::BodyOutputComposer*>(ctx)->onReflexIntent(intent);
  }, &composer);
  for (auto type : {nerve::NeuronType::TOUCH, nerve::NeuronType::LIFT_STARTED,
                    nerve::NeuronType::SHAKE}) {
    behavior.begin(0); composer.begin(0);
    context.captured_ms = 1000;
    auto event = nerve::makeNeuron(type, nerve::NeuronSource::IMU, 1000);
    behavior.onNeuron(event, context);
    reflex.onNeuron(event);
    behavior.tick(1040, context);
    const auto base = behavior.microBehavior();
    const auto active = composer.compose(base, 1040).face;
    assert(base.left_shape.lower_lid == 0 && base.jitter_x == 0);
    if (type == nerve::NeuronType::TOUCH) assert(active.left.lower_lid > 0);
    if (type == nerve::NeuronType::LIFT_STARTED) assert(active.left.height_scale > 1);
    if (type == nerve::NeuronType::SHAKE)
      assert(active.jitter_x != 0 && active.left.upper_lid > 0);
    const auto expired = composer.compose(base, 1800).face;
    assert(expired.jitter_x == base.jitter_x);
    assert(expired.left.lower_lid == base.left_shape.lower_lid);
    assert(expired.left.height_scale == base.left_shape.height_scale);
  }
  // Image X is opposite screen side; visual shape and neck show direction.
  for (int x : {-800, 0, 800}) {
    composer.begin(0);
    nerve::NeuronPayload payload; payload.x = x;
    reflex.onNeuron(nerve::makeNeuron(nerve::NeuronType::MOTION_DETECTED,
                     nerve::NeuronSource::CAMERA_M5, 1000, 1.0f, payload));
    ghost::MicroBehaviorFrame base;
    auto directed = composer.compose(base, 1200);
    assert(directed.face.offset_x == 0);
    if (x < 0) {
      assert(directed.face.right.height_scale > 1);
      assert(directed.face.left.height_scale < 1 && directed.neck_yaw > 0);
    } else if (x > 0) {
      assert(directed.face.left.height_scale > 1);
      assert(directed.face.right.height_scale < 1 && directed.neck_yaw < 0);
    } else {
      assert(directed.face.left.height_scale == 1);
      assert(directed.face.right.height_scale == 1);
    }
    base.interpersonal_priority = true;
    assert(composer.compose(base, 1200).face.left.height_scale == 1);
  }
  // Safety interrupts vision and cannot be overwritten by a later Touch.
  composer.begin(0);
  reflex.onNeuron(nerve::makeNeuron(nerve::NeuronType::SHAKE,
                                  nerve::NeuronSource::IMU, 1000));
  reflex.onNeuron(nerve::makeNeuron(nerve::NeuronType::TOUCH,
                                  nerve::NeuronSource::TOUCH, 1010));
  reflex.onNeuron(nerve::makeNeuron(nerve::NeuronType::MOTION_DETECTED,
                                  nerve::NeuronSource::CAMERA_M5, 1020));
  auto safety = composer.compose({}, 1040).face;
  assert(safety.jitter_x != 0 && safety.left.upper_lid > 0);
  assert(safety.left.lower_lid == 0);

  // Heart-driven autonomous behavior: baseline curiosity chooses a look.
  ghost::BehaviorEngine autonomousBehavior;
  ghost::HeartContext curiousContext;
  autonomousBehavior.begin(0);
  autonomousBehavior.tick(14999,curiousContext);
  assert(autonomousBehavior.autonomousAction()==ghost::AutonomousAction::NONE);
  autonomousBehavior.tick(15000,curiousContext);
  assert(autonomousBehavior.autonomousAction()==ghost::AutonomousAction::CURIOUS_LOOK);
  autonomousBehavior.tick(15500,curiousContext);
  assert(autonomousBehavior.microBehavior().left_shape.width_scale > 1.02f);

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

  // 1500 ms after the latest Vision event, action resumes with its remaining time.
  autonomousBehavior.tick(17520,curiousContext);
  assert(!autonomousBehavior.autonomousPaused());
  assert(autonomousBehavior.lastAutonomousLifecycle()==ghost::AutonomousLifecycle::RESUME);
  assert(autonomousBehavior.autonomousAction()==ghost::AutonomousAction::CURIOUS_LOOK);
  autonomousBehavior.tick(17900,curiousContext);
  assert(autonomousBehavior.autonomousAction()==ghost::AutonomousAction::CURIOUS_LOOK);
  assert(autonomousBehavior.microBehavior().left_shape.width_scale > 1.02f);

  // It completes only after the remaining autonomous lifetime is actually used.
  autonomousBehavior.tick(18920,curiousContext);
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
  assert(std::fabs(boredBehavior.microBehavior().neck_yaw) > 0.05f);
  std::cout << "PASS: neutral geometry, independent lids, blink, bounds, frame timing, clock wrap, transient expiry, Reflex-to-Body direction, expiry and safety priority, autonomous Heart-driven behavior, Vision pause/resume arbitration\n";
}

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
  std::cout << "PASS: neutral geometry, independent lids, blink, bounds, frame timing, clock wrap, transient expiry\n";
}

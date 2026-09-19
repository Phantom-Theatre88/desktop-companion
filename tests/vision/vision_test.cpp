#include <cassert>
#include <vector>
#include <iostream>
#include "../../firmware/cores3_base/src/vision/CameraVisionInput.h"
#include "../../firmware/cores3_base/src/core/DesktopCompanionRuntime.h"
// Persistence is not under test; use an empty first-boot store on the host.
namespace deskbot { namespace ghost {
bool HeartPersistence::loadPrimary(HeartState&) { return false; }
bool HeartPersistence::savePrimary(const HeartState&) { return true; }
void HeartPersistence::setMicroSdBackupHandlers(HeartBackupSaveHandler, HeartBackupLoadHandler, void*) {}
bool HeartPersistence::microSdBackupAvailable() const { return false; }
bool HeartPersistence::loadBackup(HeartState&) const { return false; }
bool HeartPersistence::saveBackup(const HeartState&) const { return false; }
}}
using namespace deskbot;
int main() {
  std::vector<uint8_t> pixels(320*240*2,0);
  device::CameraFrameView f; f.data=pixels.data(); f.bytes=pixels.size();
  f.width=320; f.height=240; f.timestamp_ms=1000;
  vision::CameraVisionInput v; nerve::SemanticNeuron n;
  v.onCameraFrame(f); assert(!v.takeNeuron(n));
  f.timestamp_ms=3000; v.onCameraFrame(f); assert(!v.takeNeuron(n));
  // Uniform lighting change is brightness, not motion.
  std::fill(pixels.begin(),pixels.end(),255);
  f.timestamp_ms=5000; v.onCameraFrame(f);
  assert(v.takeNeuron(n) && n.type==nerve::NeuronType::BRIGHTER);
  assert(!v.takeNeuron(n) && v.lastSummary().motion_score==0);
  std::fill(pixels.begin(),pixels.end(),0);
  f.timestamp_ms=6000; v.onCameraFrame(f); assert(!v.takeNeuron(n));
  f.timestamp_ms=9000; std::fill(pixels.begin(),pixels.end(),255); v.onCameraFrame(f);
  assert(v.takeNeuron(n));
  f.timestamp_ms=13000; std::fill(pixels.begin(),pixels.end(),0); v.onCameraFrame(f);
  assert(v.takeNeuron(n) && n.type==nerve::NeuronType::DARKER);
  // Move a black/white boundary, keeping the overall brightness identical.
  v.reset();
  for(size_t i=0;i<pixels.size();i+=2) {
    pixels[i]=pixels[i+1]=((i/2)%320 < 160) ? 255 : 0;
  }
  f.timestamp_ms=15000; v.onCameraFrame(f); assert(!v.takeNeuron(n));
  for(auto& pixel : pixels) pixel=255-pixel;
  f.timestamp_ms=17000; v.onCameraFrame(f);
  assert(v.takeNeuron(n) && n.type==nerve::NeuronType::MOTION_DETECTED);
  assert(n.source==nerve::NeuronSource::CAMERA_M5 && n.timestamp_ms==17000);

  // Directional motion payload is normalized semantic direction, not pixels.
  // Use a small current-frame salient block so global luma shift stays below
  // brightness classification and motion remains spatial.
  v.reset();
  std::fill(pixels.begin(),pixels.end(),0);
  f.timestamp_ms=17100; v.onCameraFrame(f); assert(!v.takeNeuron(n));
  for(size_t y=60;y<180;++y) {
    for(size_t x=8;x<48;++x) {
      const size_t i=(y*320+x)*2;
      pixels[i]=pixels[i+1]=255;
    }
  }
  f.timestamp_ms=19100; v.onCameraFrame(f);
  assert(v.takeNeuron(n) && n.type==nerve::NeuronType::MOTION_DETECTED);
  assert(n.payload.x < -250);
  assert(v.lastSummary().motion_left > v.lastSummary().motion_center);
  assert(v.lastSummary().motion_left > v.lastSummary().motion_right);

  v.reset();
  std::fill(pixels.begin(),pixels.end(),0);
  f.timestamp_ms=20000; v.onCameraFrame(f); assert(!v.takeNeuron(n));
  for(size_t y=60;y<180;++y) {
    for(size_t x=272;x<312;++x) {
      const size_t i=(y*320+x)*2;
      pixels[i]=pixels[i+1]=255;
    }
  }
  f.timestamp_ms=22000; v.onCameraFrame(f);
  assert(v.takeNeuron(n) && n.type==nerve::NeuronType::MOTION_DETECTED);
  assert(n.payload.x > 250);
  assert(v.lastSummary().motion_right > v.lastSummary().motion_left);
  assert(v.lastSummary().motion_right > v.lastSummary().motion_center);
  assert(v.lastSummary().target_blob_cells >= 5);
  assert(v.lastSummary().candidate_blob_count >= 1);

  // Isolated one-cell motion is spatial noise: it may appear in the raw mask,
  // but cleanup must remove it before blob selection and no motion neuron fires.
  v.reset();
  std::fill(pixels.begin(),pixels.end(),0);
  f.timestamp_ms=23000; v.onCameraFrame(f); assert(!v.takeNeuron(n));
  for(size_t y=120;y<140;++y) {
    for(size_t x=160;x<180;++x) {
      const size_t i=(y*320+x)*2;
      pixels[i]=pixels[i+1]=255;
    }
  }
  f.timestamp_ms=25000; v.onCameraFrame(f);
  assert(!v.takeNeuron(n));
  assert(v.lastSummary().raw_changed_cells >= 1);
  assert(v.lastSummary().cleaned_changed_cells == 0);
  assert(v.lastSummary().candidate_blob_count == 0);
  assert(v.lastSummary().target_blob_cells == 0);

  // Restore a neutral motion neuron for the existing full-route assertions.
  n = nerve::makeNeuron(nerve::NeuronType::MOTION_DETECTED,
                        nerve::NeuronSource::CAMERA_M5,17000);
  n.payload.scalar = 0.25f;
  // Full route: Vision -> Runtime/Synapse -> Ghost/Memory/Behavior -> existing eye output.
  core::DesktopCompanionRuntime runtime; assert(runtime.begin(0));
  assert(runtime.synapse().bindingCount()==23);
  runtime.tick(17300); const float resting=runtime.ghost().behaviorEngine().microBehavior().eye_openness;
  const auto heartBeforeMotion = runtime.ghost().heart();
  runtime.emit(n);
  const auto heartAfterMotion = runtime.ghost().heart();
  assert(heartAfterMotion.curiosity > heartBeforeMotion.curiosity);
  assert(heartAfterMotion.attention > heartBeforeMotion.attention);
  assert(heartAfterMotion.boredom < heartBeforeMotion.boredom);
  const float firstMotionDelta =
      heartAfterMotion.attention - heartBeforeMotion.attention;
  n.timestamp_ms = 18000;
  const auto heartBeforeRepeatedMotion = runtime.ghost().heart();
  runtime.emit(n);
  const auto heartAfterRepeatedMotion = runtime.ghost().heart();
  const float repeatedMotionDelta =
      heartAfterRepeatedMotion.attention - heartBeforeRepeatedMotion.attention;
  assert(repeatedMotionDelta > 0.0f);
  assert(repeatedMotionDelta < firstMotionDelta);
  assert(runtime.ghost().heartEngine().lastImpactScale() < 1.0f);
  assert(runtime.ghost().lastNeuronType()==nerve::NeuronType::MOTION_DETECTED);
  assert(runtime.ghost().lastNeuronMs()==18000);
  assert(runtime.ghost().heartEngine().lastEventType()==nerve::NeuronType::MOTION_DETECTED);
  assert(runtime.ghost().heartEngine().lastEventMs()==18000);
  assert(runtime.ghost().memoryEngine().lastCandidate().kind==ghost::MemoryRecordKind::SEMANTIC_EVENT);
  assert(runtime.ghost().memoryEngine().lastCandidate().neuron.type==nerve::NeuronType::MOTION_DETECTED);
  assert(runtime.ghost().behaviorEngine().lastReceivedType()==nerve::NeuronType::MOTION_DETECTED);
  assert(runtime.ghost().behaviorEngine().lastReceivedMs()==18000);
  runtime.tick(18300);
  assert(runtime.ghost().memory().last_type==nerve::NeuronType::MOTION_DETECTED);
  assert(runtime.ghost().memory().event_count==2);
  assert(runtime.ghost().behaviorEngine().microBehavior().eye_openness>resting);
  const auto heartBeforeTouch = runtime.ghost().heart();
  runtime.emit(nerve::makeNeuron(nerve::NeuronType::TOUCH,nerve::NeuronSource::TOUCH,18400));
  const auto heartAfterTouch = runtime.ghost().heart();
  assert(heartAfterTouch.mood > heartBeforeTouch.mood);
  assert(heartAfterTouch.affection > heartBeforeTouch.affection);
  assert(heartAfterTouch.attention > heartBeforeTouch.attention);

  // After quiet time, temporary Heart state moves back toward the baseline.
  const auto baseline = runtime.ghost().heartEngine().baseline();
  const auto heartBeforeRecovery = runtime.ghost().heart();
  runtime.tick(38400);
  const auto heartAfterRecovery = runtime.ghost().heart();
  assert(heartAfterRecovery.mood < heartBeforeRecovery.mood);
  assert(heartAfterRecovery.mood > baseline.mood);
  assert(heartAfterRecovery.curiosity < heartBeforeRecovery.curiosity);
  assert(heartAfterRecovery.attention < heartBeforeRecovery.attention);
  assert(heartAfterRecovery.affection == heartBeforeRecovery.affection);
  assert(heartAfterRecovery.sleepiness == heartBeforeRecovery.sleepiness);

  n.timestamp_ms=38410; runtime.emit(n); runtime.tick(38420);
  assert(runtime.ghost().behaviorEngine().microBehavior().left_shape.lower_lid>0);
  // Invalid/missing frames and long gaps cannot produce stale comparisons.
  f.bytes=1; f.timestamp_ms=19000; v.onCameraFrame(f); assert(!v.takeNeuron(n));
  assert(!v.lastSummary().valid);
  f.bytes=pixels.size(); f.timestamp_ms=21000; v.onCameraFrame(f); assert(!v.takeNeuron(n));
  std::fill(pixels.begin(),pixels.end(),0); f.timestamp_ms=31000;
  v.onCameraFrame(f); assert(!v.takeNeuron(n));
  // Clock wrap is normal; compare with unsigned elapsed time.
  v.reset(); f.timestamp_ms=UINT32_MAX-1000; v.onCameraFrame(f);
  std::fill(pixels.begin(),pixels.end(),255); f.timestamp_ms=1000; v.onCameraFrame(f);
  assert(v.takeNeuron(n) && n.type==nerve::NeuronType::BRIGHTER);
  // RGB565 is high-byte-first; saturated red must be brighter than blue.
  for (size_t i=0; i<pixels.size(); i+=2) { pixels[i]=0xf8; pixels[i+1]=0; }
  v.reset(); f.timestamp_ms=2000; v.onCameraFrame(f);
  assert(v.lastSummary().average_luma==76);
  for (size_t i=0; i<pixels.size(); i+=2) { pixels[i]=0; pixels[i+1]=0x1f; }
  f.timestamp_ms=4000; v.onCameraFrame(f);
  assert(v.lastSummary().average_luma==28);
  // Changed dimensions start a fresh baseline.
  f.width=160; f.height=120; f.timestamp_ms=5000; v.onCameraFrame(f); assert(!v.takeNeuron(n));
  std::cout << "PASS: vision cleaned blobs/direction/regions, isolated-noise rejection, Heart deltas/habituation/recovery, cooldown, invalid/gap/resize/wrap, Ghost/Heart/Memory/Behavior delivery, Behavior priority\n";
}

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
  assert(runtime.ghost().lastNeuronType()==nerve::NeuronType::MOTION_DETECTED);
  assert(runtime.ghost().lastNeuronMs()==17000);
  assert(runtime.ghost().heartEngine().lastEventType()==nerve::NeuronType::MOTION_DETECTED);
  assert(runtime.ghost().heartEngine().lastEventMs()==17000);
  assert(runtime.ghost().memoryEngine().lastCandidate().kind==ghost::MemoryRecordKind::SEMANTIC_EVENT);
  assert(runtime.ghost().memoryEngine().lastCandidate().neuron.type==nerve::NeuronType::MOTION_DETECTED);
  assert(runtime.ghost().behaviorEngine().lastReceivedType()==nerve::NeuronType::MOTION_DETECTED);
  assert(runtime.ghost().behaviorEngine().lastReceivedMs()==17000);
  runtime.tick(17300);
  assert(runtime.ghost().memory().last_type==nerve::NeuronType::MOTION_DETECTED);
  assert(runtime.ghost().memory().event_count==1);
  assert(runtime.ghost().behaviorEngine().microBehavior().eye_openness>resting);
  const auto heartBeforeTouch = runtime.ghost().heart();
  runtime.emit(nerve::makeNeuron(nerve::NeuronType::TOUCH,nerve::NeuronSource::TOUCH,17400));
  const auto heartAfterTouch = runtime.ghost().heart();
  assert(heartAfterTouch.mood > heartBeforeTouch.mood);
  assert(heartAfterTouch.affection > heartBeforeTouch.affection);
  assert(heartAfterTouch.attention > heartBeforeTouch.attention);
  n.timestamp_ms=17410; runtime.emit(n); runtime.tick(17420);
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
  std::cout << "PASS: vision changes, Heart deltas, cooldown, invalid/gap/resize/wrap, Ghost/Heart/Memory/Behavior delivery, Behavior priority\n";
}

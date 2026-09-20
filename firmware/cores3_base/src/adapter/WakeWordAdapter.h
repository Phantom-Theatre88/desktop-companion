#pragma once

#include <Arduino.h>
#include "../device/CoreS3MicDriver.h"
#include "../nerve/NerveTypes.h"

namespace deskbot {
namespace adapter {

// LOCK 59 boundary. The detector implementation (e.g. ESP-SR/WakeNet model)
// remains replaceable and may not leak into Ghost / Behavior.
using WakeWordDetectHandler = bool (*)(
    const int16_t* pcm,
    size_t sample_count,
    uint32_t sample_rate,
    float& out_confidence,
    void* context);

class WakeWordAdapter {
 public:
  void begin(const char* target_word);
  void setDetector(WakeWordDetectHandler handler, void* context);

  bool available() const { return detector_ != nullptr; }
  const char* targetWord() const { return target_word_; }

  bool toNeuron(const device::MicFrameView& frame,
                nerve::SemanticNeuron& out_neuron);

 private:
  WakeWordDetectHandler detector_ = nullptr;
  void* detector_context_ = nullptr;
  const char* target_word_ = "kibi";
};

}  // namespace adapter
}  // namespace deskbot

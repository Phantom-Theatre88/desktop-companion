#pragma once

#include <Arduino.h>
#include "../device/CoreS3MicDriver.h"
#include "../nerve/NerveTypes.h"

namespace deskbot {
namespace adapter {

class VoiceActivityAdapter {
 public:
  void begin(uint32_t now_ms);
  bool toNeuron(const device::MicFrameView& frame,
                nerve::SemanticNeuron& out_neuron);

  float debugRms() const { return last_rms_; }
  float debugPeak() const { return last_peak_; }
  float debugNoiseFloor() const { return noise_floor_; }
  bool voiceActive() const { return voice_active_; }

 private:
  static float clamp01(float value);
  static void measure(const int16_t* pcm,
                      size_t count,
                      float& out_rms,
                      float& out_peak);

  float noise_floor_ = 0.008f;
  float last_rms_ = 0.0f;
  float last_peak_ = 0.0f;
  uint8_t speech_frames_ = 0;
  uint8_t quiet_frames_ = 0;
  bool voice_active_ = false;
  uint32_t last_voice_event_ms_ = 0;
  uint32_t last_loud_event_ms_ = 0;
};

}  // namespace adapter
}  // namespace deskbot

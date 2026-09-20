#pragma once

#include <Arduino.h>
#include <M5Unified.h>

namespace deskbot {
namespace device {

struct MicFrameView {
  const int16_t* pcm = nullptr;
  size_t sample_count = 0;
  uint32_t sample_rate = 16000;
  uint32_t timestamp_ms = 0;
  bool available = false;
  bool valid = false;
};

class CoreS3MicDriver {
 public:
  bool begin(uint32_t now_ms);
  void end();
  bool poll(uint32_t now_ms, MicFrameView& out_frame);

  bool available() const { return available_; }

 private:
  static constexpr size_t kFrameSamples = 320;   // 20 ms @ 16 kHz.
  static constexpr uint32_t kSampleRate = 16000;
  static constexpr uint32_t kCodecWarmupMs = 1000;

  bool queueNext();

  int16_t buffers_[2][kFrameSamples] = {};
  uint8_t active_buffer_ = 0;
  bool recording_pending_ = false;
  bool available_ = false;
  uint32_t ready_after_ms_ = 0;
};

}  // namespace device
}  // namespace deskbot

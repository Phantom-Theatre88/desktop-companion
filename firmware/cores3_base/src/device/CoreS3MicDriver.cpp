#include "CoreS3MicDriver.h"

namespace deskbot {
namespace device {

bool CoreS3MicDriver::begin(uint32_t now_ms) {
  available_ = false;
  recording_pending_ = false;
  active_buffer_ = 0;

  // M5Unified documents that Mic and Speaker share the audio hardware path.
  // The current DeskRobo has no active TTS path yet, so hearing owns it.
  M5.Speaker.end();

  if (!M5.Mic.begin() || !M5.Mic.isEnabled()) {
    return false;
  }

  available_ = true;
  ready_after_ms_ = now_ms + kCodecWarmupMs;
  return queueNext();
}

void CoreS3MicDriver::end() {
  if (M5.Mic.isRunning()) {
    M5.Mic.end();
  }
  recording_pending_ = false;
  available_ = false;
}

bool CoreS3MicDriver::poll(uint32_t now_ms, MicFrameView& out_frame) {
  out_frame = MicFrameView{};
  out_frame.available = available_;
  if (!available_) {
    return false;
  }

  if (!recording_pending_) {
    queueNext();
    return false;
  }

  if (M5.Mic.isRecording() != 0) {
    return false;
  }

  const uint8_t completed_buffer = active_buffer_;
  recording_pending_ = false;

  out_frame.pcm = buffers_[completed_buffer];
  out_frame.sample_count = kFrameSamples;
  out_frame.sample_rate = kSampleRate;
  out_frame.timestamp_ms = now_ms;
  out_frame.valid =
      static_cast<int32_t>(now_ms - ready_after_ms_) >= 0;

  // Queue into the other buffer so the completed frame remains stable while
  // Adapter / WakeWord processing consumes it during this loop iteration.
  active_buffer_ = static_cast<uint8_t>(1U - completed_buffer);
  queueNext();
  return out_frame.valid;
}

bool CoreS3MicDriver::queueNext() {
  if (!available_ || recording_pending_) {
    return false;
  }

  recording_pending_ = M5.Mic.record(
      buffers_[active_buffer_],
      kFrameSamples,
      kSampleRate,
      false);
  return recording_pending_;
}

}  // namespace device
}  // namespace deskbot

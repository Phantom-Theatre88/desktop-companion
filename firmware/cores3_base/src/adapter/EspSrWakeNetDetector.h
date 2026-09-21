#pragma once

#include <Arduino.h>

#include "WakeWordAdapter.h"

namespace deskbot {
namespace adapter {

// Production wake-word backend boundary for LOCK 59.
//
// Unlike the earlier MultiNet command recognizer, this class runs ESP-SR in
// SR_MODE_WAKEWORD and listens for SR_EVENT_WAKEWORD from WakeNet.
//
// IMPORTANT:
// - WakeNet model identity lives in the ESP-SR model partition.
// - Until a custom model trained for "kibi" is installed, detections are
//   diagnostic only and MUST NOT become WAKE_WORD_DETECTED(kibi).
// - Once the model partition is confirmed to contain the custom kibi WakeNet
//   model, begin(true) enables semantic delivery without changing Ghost,
//   Synapse, Heart or Behavior.
struct EspSrWakeNetDiagnostics {
  uint32_t input_frames = 0;
  uint32_t input_bytes = 0;
  uint32_t dropped_frames = 0;
  uint32_t fill_calls = 0;
  uint32_t fill_bytes = 0;
  uint32_t fill_timeouts = 0;
  uint32_t sr_events = 0;
  uint32_t wake_events = 0;
  uint32_t detections_consumed = 0;
  int last_event = -1;
};

class EspSrWakeNetDetector {
 public:
  bool begin(bool kibi_model_installed);
  void end();

  bool available() const { return available_; }
  bool semanticReady() const {
    return available_ && kibi_model_installed_;
  }
  const char* backendName() const;
  const char* unavailableReason() const { return unavailable_reason_; }
  EspSrWakeNetDiagnostics diagnostics() const;

  static bool handler(const int16_t* pcm,
                      size_t sample_count,
                      uint32_t sample_rate,
                      float& out_confidence,
                      void* context);

 private:
  bool process(const int16_t* pcm,
               size_t sample_count,
               uint32_t sample_rate,
               float& out_confidence);

  bool available_ = false;
  bool kibi_model_installed_ = false;
  const char* unavailable_reason_ = "not initialized";
};

}  // namespace adapter
}  // namespace deskbot

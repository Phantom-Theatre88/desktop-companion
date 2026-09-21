#pragma once

#include <Arduino.h>

#include "WakeWordAdapter.h"

namespace deskbot {
namespace adapter {

// LOCK 59 implementation bridge.
//
// "kibi" remains a WakeWord semantic event, but the first practical CoreS3
// recognizer uses ESP-SR MultiNet in single-command mode. This gives us a
// user-defined spoken phrase without pretending that a custom WakeNet model
// already exists. The implementation stays behind WakeWordAdapter, so a true
// custom WakeNet model can replace it later without touching Ghost/Synapse.
struct EspSrKibiDiagnostics {
  uint32_t input_frames = 0;
  uint32_t input_bytes = 0;
  uint32_t dropped_frames = 0;
  uint32_t fill_calls = 0;
  uint32_t fill_bytes = 0;
  uint32_t fill_timeouts = 0;
  uint32_t sr_events = 0;
  uint32_t command_events = 0;
  uint32_t timeout_events = 0;
  uint32_t detections_consumed = 0;
  int last_event = -1;
  int last_command_id = -1;
  int last_phrase_id = -1;
};

class EspSrKibiDetector {
 public:
  bool begin();
  void end();

  bool available() const { return available_; }
  const char* backendName() const;
  const char* unavailableReason() const { return unavailable_reason_; }
  EspSrKibiDiagnostics diagnostics() const;

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
  const char* unavailable_reason_ = "not initialized";
};

}  // namespace adapter
}  // namespace deskbot

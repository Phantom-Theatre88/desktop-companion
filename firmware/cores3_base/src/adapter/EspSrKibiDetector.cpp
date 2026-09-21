#include "EspSrKibiDetector.h"

#if defined(CONFIG_IDF_TARGET_ESP32S3) && \
    (defined(CONFIG_MODEL_IN_FLASH) || defined(CONFIG_MODEL_IN_SDCARD)) && \
    (defined(ARDUINO_PARTITION_esp_sr_8) || \
     defined(ARDUINO_PARTITION_esp_sr_16) || \
     defined(ARDUINO_PARTITION_esp_sr_32)) && \
    __has_include("esp32-hal-sr.h") && __has_include(<ESP_SR.h>)

#define DESKBOT_HAS_ESP_SR_KIBI 1

// Pull in Arduino-ESP32's ESP_SR library explicitly. Besides exposing the
// speech-recognition wrapper, this is what makes the Arduino build hook copy
// the bundled srmodels.bin into the sketch build directory for esp_sr_*
// partition uploads. Without this dependency, compilation can succeed but
// upload fails because srmodels.bin is missing.
#include <ESP_SR.h>
#include "esp32-hal-sr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/stream_buffer.h"

namespace {

constexpr int kKibiCommandId = 1;
constexpr size_t kPcmStreamBytes = 16 * 1024;

// MultiNet accepts user-defined English command strings. Keeping the literal
// project wake word here lets Espressif's bundled G2P resolve the pronunciation.
// If real-device recognition proves the grapheme pronunciation unsuitable, the
// implementation may use a phonetic alias without changing the semantic word.
static const sr_cmd_t kCommands[] = {
    {kKibiCommandId, "kibi"},
};

struct EspSrState {
  StreamBufferHandle_t pcm_stream = nullptr;
  volatile bool detected = false;

  // Read-only diagnostics exported through EspSrKibiDetector::diagnostics().
  // These counters let us distinguish Mic feed, ESP-SR consumption, callback
  // activity and final command delivery without changing recognition behavior.
  volatile uint32_t input_frames = 0;
  volatile uint32_t input_bytes = 0;
  volatile uint32_t dropped_frames = 0;
  volatile uint32_t fill_calls = 0;
  volatile uint32_t fill_bytes = 0;
  volatile uint32_t fill_timeouts = 0;
  volatile uint32_t sr_events = 0;
  volatile uint32_t command_events = 0;
  volatile uint32_t timeout_events = 0;
  volatile uint32_t detections_consumed = 0;
  volatile int last_event = -1;
  volatile int last_command_id = -1;
  volatile int last_phrase_id = -1;
};

EspSrState g_state;

esp_err_t fillPcm(void* arg,
                  void* out,
                  size_t len,
                  size_t* bytes_read,
                  uint32_t timeout_ms) {
  auto* state = static_cast<EspSrState*>(arg);
  if (state == nullptr || state->pcm_stream == nullptr ||
      out == nullptr || bytes_read == nullptr) {
    return ESP_ERR_INVALID_ARG;
  }

  ++state->fill_calls;
  uint8_t* dst = static_cast<uint8_t*>(out);
  size_t total = 0;
  const TickType_t wait =
      timeout_ms == portMAX_DELAY
          ? portMAX_DELAY
          : pdMS_TO_TICKS(timeout_ms);

  while (total < len) {
    const size_t got = xStreamBufferReceive(
        state->pcm_stream,
        dst + total,
        len - total,
        wait);
    if (got == 0) {
      *bytes_read = total;
      state->fill_bytes += total;
      ++state->fill_timeouts;
      return ESP_ERR_TIMEOUT;
    }
    total += got;
  }

  *bytes_read = total;
  state->fill_bytes += total;
  return ESP_OK;
}

void onSrEvent(void* arg,
               sr_event_t event,
               int command_id,
               int phrase_id) {
  (void)phrase_id;
  auto* state = static_cast<EspSrState*>(arg);
  if (state == nullptr) {
    return;
  }

  ++state->sr_events;
  state->last_event = static_cast<int>(event);
  state->last_command_id = command_id;
  state->last_phrase_id = phrase_id;

  if (event == SR_EVENT_COMMAND) {
    ++state->command_events;
    if (command_id == kKibiCommandId) {
      state->detected = true;
    }
    // Keep continuous single-command listening.
    sr_set_mode(SR_MODE_COMMAND);
  } else if (event == SR_EVENT_TIMEOUT) {
    ++state->timeout_events;
    sr_set_mode(SR_MODE_COMMAND);
  }
}

}  // namespace

#else

#define DESKBOT_HAS_ESP_SR_KIBI 0

#endif

namespace deskbot {
namespace adapter {

bool EspSrKibiDetector::begin() {
  available_ = false;

#if DESKBOT_HAS_ESP_SR_KIBI
  if (g_state.pcm_stream == nullptr) {
    g_state.pcm_stream =
        xStreamBufferCreate(kPcmStreamBytes, sizeof(int16_t));
    if (g_state.pcm_stream == nullptr) {
      unavailable_reason_ = "ESP-SR PCM stream allocation failed";
      return false;
    }
  } else {
    xStreamBufferReset(g_state.pcm_stream);
  }

  g_state.detected = false;
  g_state.input_frames = 0;
  g_state.input_bytes = 0;
  g_state.dropped_frames = 0;
  g_state.fill_calls = 0;
  g_state.fill_bytes = 0;
  g_state.fill_timeouts = 0;
  g_state.sr_events = 0;
  g_state.command_events = 0;
  g_state.timeout_events = 0;
  g_state.detections_consumed = 0;
  g_state.last_event = -1;
  g_state.last_command_id = -1;
  g_state.last_phrase_id = -1;

  const esp_err_t err = sr_start(
      fillPcm,
      &g_state,
      SR_CHANNELS_MONO,
      SR_MODE_COMMAND,
      "M",
      kCommands,
      sizeof(kCommands) / sizeof(kCommands[0]),
      onSrEvent,
      &g_state);

  if (err != ESP_OK) {
    unavailable_reason_ = "ESP-SR sr_start failed";
    return false;
  }

  available_ = true;
  unavailable_reason_ = "";
  return true;
#else
  unavailable_reason_ =
      "select an ESP-SR partition (esp_sr_8/16/32) in Arduino IDE";
  return false;
#endif
}

void EspSrKibiDetector::end() {
#if DESKBOT_HAS_ESP_SR_KIBI
  if (available_) {
    sr_stop();
  }
  g_state.detected = false;
  if (g_state.pcm_stream != nullptr) {
    xStreamBufferReset(g_state.pcm_stream);
  }
#endif
  available_ = false;
}

const char* EspSrKibiDetector::backendName() const {
#if DESKBOT_HAS_ESP_SR_KIBI
  return "ESP-SR MultiNet single-command";
#else
  return "unavailable";
#endif
}


EspSrKibiDiagnostics EspSrKibiDetector::diagnostics() const {
  EspSrKibiDiagnostics out;
#if DESKBOT_HAS_ESP_SR_KIBI
  out.input_frames = g_state.input_frames;
  out.input_bytes = g_state.input_bytes;
  out.dropped_frames = g_state.dropped_frames;
  out.fill_calls = g_state.fill_calls;
  out.fill_bytes = g_state.fill_bytes;
  out.fill_timeouts = g_state.fill_timeouts;
  out.sr_events = g_state.sr_events;
  out.command_events = g_state.command_events;
  out.timeout_events = g_state.timeout_events;
  out.detections_consumed = g_state.detections_consumed;
  out.last_event = g_state.last_event;
  out.last_command_id = g_state.last_command_id;
  out.last_phrase_id = g_state.last_phrase_id;
#endif
  return out;
}

bool EspSrKibiDetector::handler(
    const int16_t* pcm,
    size_t sample_count,
    uint32_t sample_rate,
    float& out_confidence,
    void* context) {
  auto* self = static_cast<EspSrKibiDetector*>(context);
  if (self == nullptr) {
    out_confidence = 0.0f;
    return false;
  }
  return self->process(
      pcm, sample_count, sample_rate, out_confidence);
}

bool EspSrKibiDetector::process(
    const int16_t* pcm,
    size_t sample_count,
    uint32_t sample_rate,
    float& out_confidence) {
  out_confidence = 0.0f;

#if DESKBOT_HAS_ESP_SR_KIBI
  if (!available_ || g_state.pcm_stream == nullptr ||
      pcm == nullptr || sample_count == 0 ||
      sample_rate != 16000) {
    return false;
  }

  const size_t bytes = sample_count * sizeof(int16_t);
  ++g_state.input_frames;
  g_state.input_bytes += bytes;
  const size_t sent = xStreamBufferSend(
      g_state.pcm_stream,
      pcm,
      bytes,
      0);

  // Never turn a buffer-overflow condition into a semantic detection.
  // Audio may be dropped, but "kibi" is emitted only by ESP-SR.
  if (sent != bytes) {
    ++g_state.dropped_frames;
    return false;
  }

  if (!g_state.detected) {
    return false;
  }

  g_state.detected = false;
  ++g_state.detections_consumed;
  // Arduino's sr_event callback exposes command identity but not the internal
  // MultiNet probability. Treat a confirmed command event as full detector
  // confidence rather than fabricating a numeric model score.
  out_confidence = 1.0f;
  return true;
#else
  (void)pcm;
  (void)sample_count;
  (void)sample_rate;
  return false;
#endif
}

}  // namespace adapter
}  // namespace deskbot

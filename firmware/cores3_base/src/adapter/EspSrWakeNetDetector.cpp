#include "EspSrWakeNetDetector.h"

#if defined(CONFIG_IDF_TARGET_ESP32S3) && \
    (defined(CONFIG_MODEL_IN_FLASH) || defined(CONFIG_MODEL_IN_SDCARD)) && \
    (defined(ARDUINO_PARTITION_esp_sr_8) || \
     defined(ARDUINO_PARTITION_esp_sr_16) || \
     defined(ARDUINO_PARTITION_esp_sr_32)) && \
    __has_include("esp32-hal-sr.h") && __has_include(<ESP_SR.h>)

#define DESKBOT_HAS_ESP_SR_WAKENET 1

#include <ESP_SR.h>
#include "esp32-hal-sr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/stream_buffer.h"

namespace {

constexpr size_t kPcmStreamBytes = 16 * 1024;

struct WakeNetState {
  StreamBufferHandle_t pcm_stream = nullptr;
  volatile bool detected = false;
  volatile uint32_t input_frames = 0;
  volatile uint32_t input_bytes = 0;
  volatile uint32_t dropped_frames = 0;
  volatile uint32_t fill_calls = 0;
  volatile uint32_t fill_bytes = 0;
  volatile uint32_t fill_timeouts = 0;
  volatile uint32_t sr_events = 0;
  volatile uint32_t wake_events = 0;
  volatile uint32_t detections_consumed = 0;
  volatile int last_event = -1;
};

WakeNetState g_wakenet;

esp_err_t fillPcm(void* arg,
                  void* out,
                  size_t len,
                  size_t* bytes_read,
                  uint32_t timeout_ms) {
  auto* state = static_cast<WakeNetState*>(arg);
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
  (void)command_id;
  (void)phrase_id;

  auto* state = static_cast<WakeNetState*>(arg);
  if (state == nullptr) {
    return;
  }

  ++state->sr_events;
  state->last_event = static_cast<int>(event);

  if (event == SR_EVENT_WAKEWORD) {
    ++state->wake_events;
    state->detected = true;
  }
}

}  // namespace

#else

#define DESKBOT_HAS_ESP_SR_WAKENET 0

#endif

namespace deskbot {
namespace adapter {

bool EspSrWakeNetDetector::begin(bool kibi_model_installed) {
  available_ = false;
  kibi_model_installed_ = kibi_model_installed;

#if DESKBOT_HAS_ESP_SR_WAKENET
  if (g_wakenet.pcm_stream == nullptr) {
    g_wakenet.pcm_stream =
        xStreamBufferCreate(kPcmStreamBytes, sizeof(int16_t));
    if (g_wakenet.pcm_stream == nullptr) {
      unavailable_reason_ = "ESP-SR WakeNet PCM stream allocation failed";
      return false;
    }
  } else {
    xStreamBufferReset(g_wakenet.pcm_stream);
  }

  g_wakenet.detected = false;
  g_wakenet.input_frames = 0;
  g_wakenet.input_bytes = 0;
  g_wakenet.dropped_frames = 0;
  g_wakenet.fill_calls = 0;
  g_wakenet.fill_bytes = 0;
  g_wakenet.fill_timeouts = 0;
  g_wakenet.sr_events = 0;
  g_wakenet.wake_events = 0;
  g_wakenet.detections_consumed = 0;
  g_wakenet.last_event = -1;

  // No MultiNet command table: this is genuine WakeNet mode.
  const esp_err_t err = sr_start(
      fillPcm,
      &g_wakenet,
      SR_CHANNELS_MONO,
      SR_MODE_WAKEWORD,
      "M",
      nullptr,
      0,
      onSrEvent,
      &g_wakenet);

  if (err != ESP_OK) {
    unavailable_reason_ = "ESP-SR WakeNet sr_start failed";
    return false;
  }

  available_ = true;
  unavailable_reason_ =
      kibi_model_installed_
          ? ""
          : "WakeNet running in diagnostic-only mode: custom kibi model not installed";
  return true;
#else
  unavailable_reason_ =
      "select an ESP-SR partition (esp_sr_8/16/32) in Arduino IDE";
  return false;
#endif
}

void EspSrWakeNetDetector::end() {
#if DESKBOT_HAS_ESP_SR_WAKENET
  if (available_) {
    sr_stop();
  }
  g_wakenet.detected = false;
  if (g_wakenet.pcm_stream != nullptr) {
    xStreamBufferReset(g_wakenet.pcm_stream);
  }
#endif
  available_ = false;
  kibi_model_installed_ = false;
}

const char* EspSrWakeNetDetector::backendName() const {
#if DESKBOT_HAS_ESP_SR_WAKENET
  return "ESP-SR WakeNet";
#else
  return "unavailable";
#endif
}

EspSrWakeNetDiagnostics EspSrWakeNetDetector::diagnostics() const {
  EspSrWakeNetDiagnostics out;
#if DESKBOT_HAS_ESP_SR_WAKENET
  out.input_frames = g_wakenet.input_frames;
  out.input_bytes = g_wakenet.input_bytes;
  out.dropped_frames = g_wakenet.dropped_frames;
  out.fill_calls = g_wakenet.fill_calls;
  out.fill_bytes = g_wakenet.fill_bytes;
  out.fill_timeouts = g_wakenet.fill_timeouts;
  out.sr_events = g_wakenet.sr_events;
  out.wake_events = g_wakenet.wake_events;
  out.detections_consumed = g_wakenet.detections_consumed;
  out.last_event = g_wakenet.last_event;
#endif
  return out;
}

bool EspSrWakeNetDetector::handler(
    const int16_t* pcm,
    size_t sample_count,
    uint32_t sample_rate,
    float& out_confidence,
    void* context) {
  auto* self = static_cast<EspSrWakeNetDetector*>(context);
  if (self == nullptr) {
    out_confidence = 0.0f;
    return false;
  }
  return self->process(
      pcm, sample_count, sample_rate, out_confidence);
}

bool EspSrWakeNetDetector::process(
    const int16_t* pcm,
    size_t sample_count,
    uint32_t sample_rate,
    float& out_confidence) {
  out_confidence = 0.0f;

#if DESKBOT_HAS_ESP_SR_WAKENET
  if (!available_ || g_wakenet.pcm_stream == nullptr ||
      pcm == nullptr || sample_count == 0 ||
      sample_rate != 16000) {
    return false;
  }

  const size_t bytes = sample_count * sizeof(int16_t);
  ++g_wakenet.input_frames;
  g_wakenet.input_bytes += bytes;

  const size_t sent = xStreamBufferSend(
      g_wakenet.pcm_stream,
      pcm,
      bytes,
      0);

  if (sent != bytes) {
    ++g_wakenet.dropped_frames;
    return false;
  }

  if (!g_wakenet.detected) {
    return false;
  }

  g_wakenet.detected = false;

  // Never claim that a bundled/default WakeNet phrase means "kibi".
  // Semantic delivery is enabled only after the installed model partition is
  // explicitly confirmed as the custom kibi model.
  if (!kibi_model_installed_) {
    return false;
  }

  ++g_wakenet.detections_consumed;
  // ESP_SR's low-level wake callback exposes detection identity, not a model
  // probability. Do not fabricate a probability; 1.0 means a confirmed model
  // event crossed this adapter boundary.
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

#include "VoiceActivityAdapter.h"

#include <math.h>

namespace deskbot {
namespace adapter {
namespace {

constexpr float kMinimumVoiceRms = 0.018f;
constexpr float kNoiseMultiplier = 2.8f;
constexpr uint8_t kSpeechFramesRequired = 3;  // ~60 ms
constexpr uint8_t kQuietFramesRequired = 8;   // ~160 ms
constexpr uint32_t kVoiceCooldownMs = 700;

constexpr float kLoudPeakThreshold = 0.80f;
constexpr float kLoudRmsThreshold = 0.10f;
constexpr uint32_t kLoudCooldownMs = 1000;

constexpr float kNoiseLearnRate = 0.025f;

}  // namespace

void VoiceActivityAdapter::begin(uint32_t now_ms) {
  noise_floor_ = 0.008f;
  last_rms_ = 0.0f;
  last_peak_ = 0.0f;
  speech_frames_ = 0;
  quiet_frames_ = 0;
  voice_active_ = false;
  last_voice_event_ms_ = now_ms - kVoiceCooldownMs;
  last_loud_event_ms_ = now_ms - kLoudCooldownMs;
}

bool VoiceActivityAdapter::toNeuron(
    const device::MicFrameView& frame,
    nerve::SemanticNeuron& out_neuron) {
  if (!frame.available || !frame.valid ||
      frame.pcm == nullptr || frame.sample_count == 0) {
    return false;
  }

  measure(frame.pcm, frame.sample_count, last_rms_, last_peak_);

  const float voice_threshold =
      noise_floor_ * kNoiseMultiplier > kMinimumVoiceRms
          ? noise_floor_ * kNoiseMultiplier
          : kMinimumVoiceRms;

  const bool speech_like = last_rms_ >= voice_threshold;

  if (!voice_active_ && !speech_like) {
    // Learn the room only while we currently believe it is quiet.
    noise_floor_ += (last_rms_ - noise_floor_) * kNoiseLearnRate;
    if (noise_floor_ < 0.002f) noise_floor_ = 0.002f;
    if (noise_floor_ > 0.060f) noise_floor_ = 0.060f;
  }

  if (last_peak_ >= kLoudPeakThreshold &&
      last_rms_ >= kLoudRmsThreshold &&
      (frame.timestamp_ms - last_loud_event_ms_) >= kLoudCooldownMs) {
    last_loud_event_ms_ = frame.timestamp_ms;
    nerve::NeuronPayload payload;
    payload.scalar = clamp01(last_rms_ / 0.35f);
    out_neuron = nerve::makeNeuron(
        nerve::NeuronType::LOUD_SOUND,
        nerve::NeuronSource::MIC,
        frame.timestamp_ms,
        clamp01(0.70f + payload.scalar * 0.30f),
        payload);
    return true;
  }

  if (speech_like) {
    quiet_frames_ = 0;
    if (speech_frames_ < 255) ++speech_frames_;

    if (!voice_active_ &&
        speech_frames_ >= kSpeechFramesRequired) {
      voice_active_ = true;
      if ((frame.timestamp_ms - last_voice_event_ms_) >= kVoiceCooldownMs) {
        last_voice_event_ms_ = frame.timestamp_ms;
        nerve::NeuronPayload payload;
        payload.scalar = clamp01(last_rms_ / 0.20f);
        out_neuron = nerve::makeNeuron(
            nerve::NeuronType::VOICE_ACTIVITY,
            nerve::NeuronSource::MIC,
            frame.timestamp_ms,
            clamp01(0.55f + payload.scalar * 0.40f),
            payload);
        return true;
      }
    }
  } else {
    speech_frames_ = 0;
    if (voice_active_) {
      if (quiet_frames_ < 255) ++quiet_frames_;
      if (quiet_frames_ >= kQuietFramesRequired) {
        voice_active_ = false;
        quiet_frames_ = 0;
      }
    }
  }

  return false;
}

float VoiceActivityAdapter::clamp01(float value) {
  if (value < 0.0f) return 0.0f;
  if (value > 1.0f) return 1.0f;
  return value;
}

void VoiceActivityAdapter::measure(const int16_t* pcm,
                                   size_t count,
                                   float& out_rms,
                                   float& out_peak) {
  if (pcm == nullptr || count == 0) {
    out_rms = 0.0f;
    out_peak = 0.0f;
    return;
  }

  int64_t sum = 0;
  for (size_t i = 0; i < count; ++i) {
    sum += pcm[i];
  }
  const float mean = static_cast<float>(sum) / static_cast<float>(count);

  double square_sum = 0.0;
  float peak = 0.0f;
  for (size_t i = 0; i < count; ++i) {
    const float centered =
        static_cast<float>(pcm[i]) - mean;
    const float normalized = centered / 32768.0f;
    square_sum += static_cast<double>(normalized) *
                  static_cast<double>(normalized);
    const float abs_value = normalized < 0.0f ? -normalized : normalized;
    if (abs_value > peak) peak = abs_value;
  }

  out_rms = sqrtf(
      static_cast<float>(square_sum / static_cast<double>(count)));
  out_peak = peak;
}

}  // namespace adapter
}  // namespace deskbot

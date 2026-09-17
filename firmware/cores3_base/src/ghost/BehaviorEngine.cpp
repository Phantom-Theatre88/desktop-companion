#include "BehaviorEngine.h"

#include <math.h>

namespace deskbot {
namespace ghost {

namespace {

// LOCK 52: these are implementation defaults for the first standalone DeskRobo
// life loop. They are tuning parameters, not personality LOCK values.
constexpr uint32_t kBlinkPeriodMs = 4200;
constexpr uint32_t kBlinkCloseMs = 90;
constexpr uint32_t kBlinkHoldMs = 45;
constexpr uint32_t kBlinkOpenMs = 110;
constexpr uint32_t kTouchResponseMs = 500;
constexpr uint32_t kPickedUpResponseMs = 700;
constexpr uint32_t kShakeResponseMs = 650;

}  // namespace

void BehaviorEngine::begin(uint32_t now_ms) {
  started_ms_ = now_ms;
  last_event_ms_ = now_ms;
  last_event_type_ = nerve::NeuronType::NONE;
  micro_behavior_ = MicroBehaviorFrame{};
  micro_behavior_.generated_ms = now_ms;
}

void BehaviorEngine::onNeuron(const nerve::SemanticNeuron& neuron,
                              const HeartContext& event_context) {
  (void)event_context;
  last_event_ms_ = neuron.timestamp_ms;
  last_event_type_ = neuron.type;
}

void BehaviorEngine::tick(uint32_t now_ms, const HeartContext& heart_context) {
  const HeartState& heart = heart_context.state;

  // A small Heart-influenced resting openness. Sleepiness closes the eyes a
  // little, while attention keeps them more awake. This is continuous output,
  // not a fixed expression preset.
  float resting_openness = clamp01(
      0.78f + (heart.attention * 0.16f) - (heart.sleepiness * 0.28f));

  // Deterministic micro gaze. We intentionally avoid pure random motion: the
  // amplitude is shaped by curiosity and attention so the motion belongs to
  // Ghost/Behavior rather than being decorative noise.
  const float t = static_cast<float>(now_ms - started_ms_) * 0.001f;
  const float gaze_energy = 0.05f + (heart.curiosity * 0.10f) + (heart.attention * 0.05f);
  micro_behavior_.gaze_x = clampSigned(sinf(t * 0.47f) * gaze_energy);
  micro_behavior_.gaze_y = clampSigned(sinf((t * 0.31f) + 1.2f) * gaze_energy * 0.45f);

  // LOCK 52: internal senses should become visible body responses through the
  // normal SemanticNeuron -> Ghost -> Behavior path. These amplitudes/times are
  // implementation tuning values, not personality LOCK values.
  const uint32_t event_age_ms = now_ms - last_event_ms_;
  const bool touch_response =
      last_event_type_ == nerve::NeuronType::TOUCH &&
      event_age_ms < kTouchResponseMs;
  const bool picked_up_response =
      last_event_type_ == nerve::NeuronType::PICKED_UP &&
      event_age_ms < kPickedUpResponseMs;
  const bool shake_response =
      last_event_type_ == nerve::NeuronType::SHAKE &&
      event_age_ms < kShakeResponseMs;

  if (touch_response) {
    resting_openness = clamp01(resting_openness + 0.10f);
    micro_behavior_.gaze_x *= 0.35f;
    micro_behavior_.gaze_y = clampSigned(micro_behavior_.gaze_y - 0.08f);
  } else if (picked_up_response) {
    // Being lifted is treated as immediate attention/arousal rather than a
    // fixed emotion preset.
    resting_openness = clamp01(resting_openness + 0.18f);
    micro_behavior_.gaze_x *= 0.20f;
    micro_behavior_.gaze_y = clampSigned(micro_behavior_.gaze_y + 0.10f);
  } else if (shake_response) {
    // A shake is a stronger body event. Keep the response procedural and
    // temporary; exact expression design remains a later Face task.
    resting_openness = clamp01(resting_openness + 0.12f);
    const float shake_phase = static_cast<float>(event_age_ms) * 0.035f;
    micro_behavior_.gaze_x = clampSigned(sinf(shake_phase) * 0.22f);
    micro_behavior_.gaze_y = clampSigned(cosf(shake_phase * 0.7f) * 0.10f);
  }

  // Blink envelope. This gives the standalone body a life rhythm without
  // requiring any external sensor. Detailed rhythm/personality tuning remains
  // intentionally adjustable later.
  const uint32_t phase = (now_ms - started_ms_) % kBlinkPeriodMs;
  const uint32_t close_end = kBlinkCloseMs;
  const uint32_t hold_end = close_end + kBlinkHoldMs;
  const uint32_t open_end = hold_end + kBlinkOpenMs;

  float blink_scale = 1.0f;
  bool blink_active = false;

  if (phase < close_end) {
    blink_active = true;
    blink_scale = 1.0f - (static_cast<float>(phase) / static_cast<float>(kBlinkCloseMs));
  } else if (phase < hold_end) {
    blink_active = true;
    blink_scale = 0.0f;
  } else if (phase < open_end) {
    blink_active = true;
    blink_scale = static_cast<float>(phase - hold_end) / static_cast<float>(kBlinkOpenMs);
  }

  micro_behavior_.blink_active = blink_active;
  micro_behavior_.eye_openness = clamp01(resting_openness * blink_scale);

  // Tiny left/right asymmetry prevents the face from looking mechanically
  // mirrored while keeping the output subtle and continuous.
  micro_behavior_.left_eye_bias = sinf((t * 0.23f) + 0.4f) * 0.025f;
  micro_behavior_.right_eye_bias = sinf((t * 0.19f) + 2.0f) * 0.025f;

  if (shake_response) {
    const float shake_bias = sinf(static_cast<float>(event_age_ms) * 0.045f) * 0.05f;
    micro_behavior_.left_eye_bias += shake_bias;
    micro_behavior_.right_eye_bias -= shake_bias;
  }

  micro_behavior_.generated_ms = now_ms;
}

float BehaviorEngine::clamp01(float value) {
  if (value < 0.0f) {
    return 0.0f;
  }
  if (value > 1.0f) {
    return 1.0f;
  }
  return value;
}

float BehaviorEngine::clampSigned(float value) {
  if (value < -1.0f) {
    return -1.0f;
  }
  if (value > 1.0f) {
    return 1.0f;
  }
  return value;
}

}  // namespace ghost
}  // namespace deskbot

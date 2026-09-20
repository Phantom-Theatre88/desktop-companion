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
constexpr uint32_t kVisualResponseMs = 1500;
constexpr uint32_t kMotionAttackMs = 120;
constexpr uint32_t kMotionHoldMs = 300;
constexpr uint32_t kMotionReleaseMs =
    kVisualResponseMs - kMotionAttackMs - kMotionHoldMs;
constexpr float kMotionDirectionDeadzone = 0.12f;
constexpr uint32_t kAutonomousDecisionIntervalMs = 15000;
constexpr uint32_t kCuriousLookMs = 1800;
constexpr uint32_t kBoredScanMs = 2600;
constexpr float kCuriousThreshold = 0.64f;
constexpr float kBoredThreshold = 0.35f;

}  // namespace

void BehaviorEngine::begin(uint32_t now_ms) {
  last_received_type_ = nerve::NeuronType::NONE;
  last_received_ms_ = 0;
  visual_event_type_ = nerve::NeuronType::NONE;
  visual_event_ms_ = now_ms;
  visual_target_x_ = 0.0f;
  visual_target_y_ = 0.0f;
  started_ms_ = now_ms;
  last_event_ms_ = now_ms;
  last_event_type_ = nerve::NeuronType::NONE;
  autonomous_action_ = AutonomousAction::NONE;
  autonomous_action_started_ms_ = now_ms;
  last_autonomous_decision_ms_ = now_ms;
  autonomous_decision_seq_ = 0;
  autonomous_sequence_ = 0;
  autonomous_direction_ = 1.0f;
  autonomous_paused_ = false;
  autonomous_pause_started_ms_ = 0;
  last_autonomous_lifecycle_ = AutonomousLifecycle::NONE;
  autonomous_lifecycle_seq_ = 0;
  autonomous_lifecycle_ms_ = now_ms;
  micro_behavior_ = MicroBehaviorFrame{};
  micro_behavior_.generated_ms = now_ms;
}

void BehaviorEngine::onNeuron(const nerve::SemanticNeuron& neuron,
                              const HeartContext& event_context) {
  const uint32_t handled_ms = event_context.captured_ms;
  last_received_type_ = neuron.type;
  last_received_ms_ = neuron.timestamp_ms;

  // LOCK 15 arbitration:
  // - Direct body interaction cancels autonomous behavior.
  // - Low-level Vision temporarily overlays the body output but preserves the
  //   autonomous decision underneath, so it can resume if time remains.
  if (neuron.type == nerve::NeuronType::TOUCH ||
      neuron.type == nerve::NeuronType::PICKED_UP ||
      neuron.type == nerve::NeuronType::SHAKE) {
    if (autonomous_action_ != AutonomousAction::NONE) {
      markAutonomousLifecycle(AutonomousLifecycle::CANCEL, handled_ms);
    }
    autonomous_action_ = AutonomousAction::NONE;
    autonomous_paused_ = false;
    autonomous_pause_started_ms_ = 0;
    last_autonomous_decision_ms_ = handled_ms;
  }

  if (neuron.type == nerve::NeuronType::MOTION_DETECTED ||
      neuron.type == nerve::NeuronType::BRIGHTER ||
      neuron.type == nerve::NeuronType::DARKER) {
    visual_event_type_ = neuron.type;
    visual_event_ms_ = handled_ms;
    if (neuron.type == nerve::NeuronType::MOTION_DETECTED) {
      visual_target_x_ = clampSigned(
          static_cast<float>(neuron.payload.x) / 1000.0f);
      visual_target_y_ = clampSigned(
          static_cast<float>(neuron.payload.y) / 1000.0f);
    } else {
      visual_target_x_ = 0.0f;
      visual_target_y_ = 0.0f;
    }
    if (autonomous_action_ != AutonomousAction::NONE && !autonomous_paused_) {
      autonomous_paused_ = true;
      autonomous_pause_started_ms_ = handled_ms;
      markAutonomousLifecycle(AutonomousLifecycle::PAUSE, handled_ms);
    }
    return;  // Vision overlays but does not discard autonomous intent.
  }
  last_event_ms_ = handled_ms;
  last_event_type_ = neuron.type;
}

void BehaviorEngine::tick(uint32_t now_ms, const HeartContext& heart_context) {
  const HeartState& heart = heart_context.state;
  // Rebuild transient geometry each tick; expired events cannot latch a face.
  micro_behavior_.left_shape = face::EyeShape{};
  micro_behavior_.right_shape = face::EyeShape{};
  micro_behavior_.eye_spacing_scale = 1.0f;
  micro_behavior_.jitter_x = micro_behavior_.jitter_y = 0.0f;

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
  micro_behavior_.gaze_x = 0.0f;
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

  const uint32_t visual_age = now_ms - visual_event_ms_;
  const bool visual_response = visual_age < kVisualResponseMs;

  bool resumed_this_tick = false;

  // Vision pauses autonomous action time. Repeated Vision events keep the pause
  // alive because visual_event_ms_ moves forward, but pause_started_ms_ remains
  // the first interruption point. When Vision clears, shift the action start
  // forward by the exact paused duration so no autonomous lifetime is lost.
  if (autonomous_paused_ && !visual_response) {
    const uint32_t paused_ms = now_ms - autonomous_pause_started_ms_;
    autonomous_action_started_ms_ += paused_ms;
    autonomous_paused_ = false;
    autonomous_pause_started_ms_ = 0;
    markAutonomousLifecycle(AutonomousLifecycle::RESUME, now_ms);
    resumed_this_tick = true;
  }

  if (!touch_response && !picked_up_response && !shake_response &&
      !visual_response &&
      autonomous_action_ == AutonomousAction::NONE &&
      (now_ms - last_autonomous_decision_ms_) >=
          kAutonomousDecisionIntervalMs) {
    chooseAutonomousAction(now_ms, heart);
  }

  if (!resumed_this_tick && !autonomous_paused_ &&
      autonomous_action_ == AutonomousAction::CURIOUS_LOOK &&
      (now_ms - autonomous_action_started_ms_) >= kCuriousLookMs) {
    markAutonomousLifecycle(AutonomousLifecycle::COMPLETE, now_ms);
    autonomous_action_ = AutonomousAction::NONE;
  } else if (!resumed_this_tick && !autonomous_paused_ &&
             autonomous_action_ == AutonomousAction::BORED_SCAN &&
             (now_ms - autonomous_action_started_ms_) >= kBoredScanMs) {
    markAutonomousLifecycle(AutonomousLifecycle::COMPLETE, now_ms);
    autonomous_action_ = AutonomousAction::NONE;
  }

  if (touch_response) {
    const float amount = 1.0f - static_cast<float>(event_age_ms) / kTouchResponseMs;
    micro_behavior_.left_shape.lower_lid = 0.22f * amount;
    micro_behavior_.left_shape.radius_scale = 1.0f + 0.20f * amount;
    // TOUCH eyelid experiment: add a gentle mirrored upper-lid slope.
    // Negative tilt uses the renderer's softer inner-edge direction, while
    // lower-lid lift remains the primary "touched" cue.
    micro_behavior_.left_shape.tilt = -0.45f * amount;
    micro_behavior_.right_shape = micro_behavior_.left_shape;
    resting_openness = clamp01(resting_openness + 0.10f);
    micro_behavior_.gaze_x *= 0.35f;
    micro_behavior_.gaze_y = clampSigned(micro_behavior_.gaze_y - 0.08f);
  } else if (picked_up_response) {
    const float amount = 1.0f - static_cast<float>(event_age_ms) / kPickedUpResponseMs;
    micro_behavior_.left_shape.height_scale = 1.0f + 0.18f * amount;
    micro_behavior_.left_shape.width_scale = 1.0f + 0.04f * amount;
    micro_behavior_.right_shape = micro_behavior_.left_shape;
    micro_behavior_.eye_spacing_scale = 1.0f + 0.025f * amount;
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
    const float amount = 1.0f - static_cast<float>(event_age_ms) / kShakeResponseMs;
    micro_behavior_.jitter_x = sinf(shake_phase) * 0.65f * amount;
    micro_behavior_.jitter_y = sinf(shake_phase * 0.7f) * 0.30f * amount;
    micro_behavior_.left_shape.upper_lid = 0.18f * amount;
    micro_behavior_.right_shape.height_scale = 1.0f + 0.12f * amount;
  }

  // Low-level Vision body response. Directional motion is intentionally
  // "fast in, slow out": make the direction obvious first, hold it briefly,
  // then let the face relax back to neutral.
  if (!touch_response && !picked_up_response && !shake_response &&
      visual_age < kVisualResponseMs) {
    float amount = 0.0f;
    if (visual_age < kMotionAttackMs) {
      amount = static_cast<float>(visual_age) /
               static_cast<float>(kMotionAttackMs);
    } else if (visual_age < kMotionAttackMs + kMotionHoldMs) {
      amount = 1.0f;
    } else {
      const uint32_t release_age =
          visual_age - kMotionAttackMs - kMotionHoldMs;
      amount = 1.0f -
          static_cast<float>(release_age) /
          static_cast<float>(kMotionReleaseMs);
    }
    amount = clamp01(amount);

    if (visual_event_type_ == nerve::NeuronType::MOTION_DETECTED) {
      resting_openness = clamp01(resting_openness + 0.10f * amount);

      // Do not translate the whole eye pair during a Vision direction cue.
      // On this pupil-less face the short travel distance reads as slow drift,
      // so direction is expressed by shape only.
      micro_behavior_.gaze_x = 0.0f;
      micro_behavior_.gaze_y = 0.0f;

      // Camera/image X and the face's screen-left/screen-right naming are
      // opposite for a robot facing the user. Therefore invert the side used
      // for the visible cue: motion on camera-right emphasizes screen-left,
      // and camera-left emphasizes screen-right.
      //
      // Use strong silhouette contrast within the existing EyeShape bounds:
      // attended side = tall + narrow, opposite side = short + wide.
      const float abs_x = visual_target_x_ < 0.0f
          ? -visual_target_x_ : visual_target_x_;
      if (abs_x >= kMotionDirectionDeadzone) {
        // Once the direction is trustworthy enough to cross the deadzone,
        // show it clearly. Do not weaken the cue merely because the blob
        // center is only moderately off-center.
        const float emphasis = amount;

        auto emphasizeScreenLeft = [&]() {
          micro_behavior_.left_shape.height_scale =
              1.0f + 0.25f * emphasis;
          micro_behavior_.left_shape.width_scale =
              1.0f - 0.20f * emphasis;
          micro_behavior_.right_shape.height_scale =
              1.0f - 0.40f * emphasis;
          micro_behavior_.right_shape.width_scale =
              1.0f + 0.20f * emphasis;
        };
        auto emphasizeScreenRight = [&]() {
          micro_behavior_.right_shape.height_scale =
              1.0f + 0.25f * emphasis;
          micro_behavior_.right_shape.width_scale =
              1.0f - 0.20f * emphasis;
          micro_behavior_.left_shape.height_scale =
              1.0f - 0.40f * emphasis;
          micro_behavior_.left_shape.width_scale =
              1.0f + 0.20f * emphasis;
        };

        if (visual_target_x_ > 0.0f) {
          emphasizeScreenLeft();
        } else {
          emphasizeScreenRight();
        }
      }
    } else if (visual_event_type_ == nerve::NeuronType::BRIGHTER) {
      const float legacy_amount = 1.0f -
          static_cast<float>(visual_age) /
          static_cast<float>(kVisualResponseMs);
      resting_openness =
          clamp01(resting_openness - 0.10f * legacy_amount);
    } else if (visual_event_type_ == nerve::NeuronType::DARKER) {
      const float legacy_amount = 1.0f -
          static_cast<float>(visual_age) /
          static_cast<float>(kVisualResponseMs);
      resting_openness =
          clamp01(resting_openness + 0.06f * legacy_amount);
    }
  }

  // Autonomous action layer. It only runs below external event responses.
  // The action is selected from Heart state, never by pure random choice.
  if (!touch_response && !picked_up_response && !shake_response &&
      !visual_response && !autonomous_paused_) {
    const uint32_t autonomous_age = now_ms - autonomous_action_started_ms_;
    if (autonomous_action_ == AutonomousAction::CURIOUS_LOOK) {
      const float p = clamp01(
          static_cast<float>(autonomous_age) / static_cast<float>(kCuriousLookMs));
      const float envelope = sinf(p * 3.14159265f);
      micro_behavior_.gaze_x = 0.0f;
      micro_behavior_.gaze_y = clampSigned(
          micro_behavior_.gaze_y - 0.08f * envelope);
      micro_behavior_.left_shape.width_scale = 1.0f + 0.06f * envelope;
      micro_behavior_.right_shape.width_scale = 1.0f + 0.06f * envelope;
      resting_openness = clamp01(resting_openness + 0.05f * envelope);
    } else if (autonomous_action_ == AutonomousAction::BORED_SCAN) {
      const float p = clamp01(
          static_cast<float>(autonomous_age) / static_cast<float>(kBoredScanMs));
      const float phase = p * 6.28318531f;
      const float scan_amount = 0.32f + heart.boredom * 0.18f;
      micro_behavior_.gaze_x = 0.0f;
      micro_behavior_.gaze_y = clampSigned(cosf(phase) * 0.05f);
      micro_behavior_.left_shape.upper_lid = 0.05f + heart.boredom * 0.05f;
      micro_behavior_.right_shape.upper_lid = micro_behavior_.left_shape.upper_lid;
    }
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

void BehaviorEngine::chooseAutonomousAction(uint32_t now_ms,
                                            const HeartState& heart) {
  last_autonomous_decision_ms_ = now_ms;
  ++autonomous_decision_seq_;
  autonomous_action_started_ms_ = now_ms;

  if (heart.boredom >= kBoredThreshold) {
    autonomous_action_ = AutonomousAction::BORED_SCAN;
    autonomous_paused_ = false;
    markAutonomousLifecycle(AutonomousLifecycle::START, now_ms);
    ++autonomous_sequence_;
    return;
  }

  if (heart.curiosity >= kCuriousThreshold && heart.attention >= 0.35f) {
    autonomous_action_ = AutonomousAction::CURIOUS_LOOK;
    autonomous_direction_ = (autonomous_sequence_ % 2 == 0) ? 1.0f : -1.0f;
    autonomous_paused_ = false;
    markAutonomousLifecycle(AutonomousLifecycle::START, now_ms);
    ++autonomous_sequence_;
    return;
  }

  // "Do nothing" is a formal decision, not a missing branch.
  autonomous_action_ = AutonomousAction::NONE;
  ++autonomous_sequence_;
}

void BehaviorEngine::markAutonomousLifecycle(
    AutonomousLifecycle lifecycle,
    uint32_t now_ms) {
  last_autonomous_lifecycle_ = lifecycle;
  autonomous_lifecycle_ms_ = now_ms;
  ++autonomous_lifecycle_seq_;
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

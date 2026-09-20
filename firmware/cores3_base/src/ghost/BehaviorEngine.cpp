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
constexpr uint32_t kVisualResponseMs = 1500;
constexpr uint32_t kAutonomousDecisionIntervalMs = 15000;
constexpr uint32_t kCuriousLookMs = 1800;
constexpr uint32_t kBoredScanMs = 2600;
constexpr uint32_t kYawnMs = 3000;
constexpr uint32_t kCoffeeBreakMs = 4200;
constexpr uint32_t kWakeHoldMs = 15000;
constexpr uint32_t kSweatAfterShakeMs = 1800;
constexpr uint32_t kNoticeEffectMs = 900;
constexpr uint32_t kQuestionEffectMs = 1400;
constexpr float kCuriousThreshold = 0.64f;
constexpr float kBoredThreshold = 0.25f;
constexpr float kDrowsySleepinessThreshold = 0.28f;
constexpr float kDrowsyBoredomThreshold = 0.20f;
constexpr float kSleepSleepinessThreshold = 0.42f;
constexpr float kSleepBoredomThreshold = 0.28f;

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
  life_state_ = LifeState::AWAKE;
  awake_hold_until_ms_ = now_ms;
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
  transient_effect_ = face::VisualEffect::NONE;
  transient_effect_started_ms_ = now_ms;
  transient_effect_duration_ms_ = 0;
  transient_effect_amount_ = 1.0f;
  micro_behavior_ = MicroBehaviorFrame{};
  micro_behavior_.generated_ms = now_ms;
}

void BehaviorEngine::onNeuron(const nerve::SemanticNeuron& neuron,
                              const HeartContext& event_context) {
  const uint32_t handled_ms = event_context.captured_ms;
  last_received_type_ = neuron.type;
  last_received_ms_ = neuron.timestamp_ms;

  // LOCK 58: symbolic effects are selected from semantic meaning + context,
  // never from a raw Heart threshold alone.
  switch (neuron.type) {
    case nerve::NeuronType::SHAKE:
      // Reflex owns the immediate body response; the sweat remains long enough
      // to appear after that strong reflex finishes.
      triggerEffect(face::VisualEffect::SWEAT,
                    handled_ms,
                    kSweatAfterShakeMs,
                    clamp01(0.55f + neuron.confidence * 0.45f));
      break;

    case nerve::NeuronType::FACE_DETECTED:
    case nerve::NeuronType::VOICE_ACTIVITY:
    case nerve::NeuronType::ATTENTION_REQUEST:
      triggerEffect(face::VisualEffect::NOTICE,
                    handled_ms,
                    kNoticeEffectMs,
                    clamp01(0.55f + neuron.confidence * 0.45f));
      break;

    case nerve::NeuronType::FACE_LOST:
      // "Where did you go?" is a contextual question, not a generic confusion
      // meter. A future recognition/understanding-failure neuron may also use
      // QUESTION without changing the renderer.
      triggerEffect(face::VisualEffect::QUESTION,
                    handled_ms,
                    kQuestionEffectMs,
                    clamp01(0.50f + neuron.confidence * 0.40f));
      break;

    default:
      break;
  }

  // Only interaction-level stimuli wake the continuing life state.
  // Low-level Vision may still affect attention/curiosity and Reflex, but an
  // ambient MOTION/BRIGHTER/DARKER event must not repeatedly wake a sleeping
  // DeskRobo. Future voice/person-recognition neurons can join this set.
  const bool wake_stimulus =
      neuron.type == nerve::NeuronType::TOUCH ||
      neuron.type == nerve::NeuronType::PICKED_UP ||
      neuron.type == nerve::NeuronType::SHAKE;
  if (wake_stimulus) {
    life_state_ = LifeState::AWAKE;
    awake_hold_until_ms_ = handled_ms + kWakeHoldMs;
  }

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
  const HeartState& baseline = heart_context.baseline;

  updateLifeState(now_ms, heart);

  // Rebuild the Heart/Behavior body frame every tick. Reflex is composed later
  // in BodyOutputComposer and must not be baked into this stream.
  micro_behavior_.left_shape = face::EyeShape{};
  micro_behavior_.right_shape = face::EyeShape{};
  micro_behavior_.eye_spacing_scale = 1.0f;
  micro_behavior_.jitter_x = micro_behavior_.jitter_y = 0.0f;
  micro_behavior_.mouth_open = 0.0f;
  micro_behavior_.prop = face::VisualProp::NONE;
  micro_behavior_.prop_progress = 0.0f;
  micro_behavior_.effect = face::VisualEffect::NONE;
  micro_behavior_.effect_progress = 0.0f;
  micro_behavior_.effect_amount = 1.0f;
  micro_behavior_.sleep_zzz = false;

  if (transient_effect_ != face::VisualEffect::NONE &&
      transient_effect_duration_ms_ > 0) {
    const uint32_t effect_age = now_ms - transient_effect_started_ms_;
    if (effect_age < transient_effect_duration_ms_) {
      micro_behavior_.effect = transient_effect_;
      micro_behavior_.effect_progress =
          clamp01(static_cast<float>(effect_age) /
                  static_cast<float>(transient_effect_duration_ms_));
      micro_behavior_.effect_amount = transient_effect_amount_;
    } else {
      transient_effect_ = face::VisualEffect::NONE;
      transient_effect_duration_ms_ = 0;
    }
  }
  micro_behavior_.sleep_zzz_phase = 0.0f;
  micro_behavior_.neck_yaw = 0.0f;
  micro_behavior_.neck_pitch = 0.0f;

  // Heart -> body is expressed as deviation from the current dynamic baseline,
  // not as fixed "happy/sad" presets. This keeps long-term personality shifts
  // compatible with LOCK 21-26 while letting temporary state continuously
  // appear in the body.
  const float mood_delta = heart.mood - baseline.mood;
  const float curiosity_delta = heart.curiosity - baseline.curiosity;
  const float boredom_delta = heart.boredom - baseline.boredom;
  const float sleepiness_delta = heart.sleepiness - baseline.sleepiness;
  const float attention_delta = heart.attention - baseline.attention;

  float resting_openness = clamp01(
      0.88f +
      (attention_delta * 0.26f) -
      (sleepiness_delta * 0.42f) -
      (boredom_delta * 0.10f) +
      (mood_delta * 0.08f));

  // Subtle continuous geometry. These are implementation tuning values, not
  // personality LOCKs. Affection remains a long-term relationship state and is
  // intentionally not rendered as a permanent facial parameter.
  const float mood_height = mood_delta * 0.10f;
  const float curiosity_width = curiosity_delta * 0.08f;
  const float tired_lid =
      sleepiness_delta > 0.0f ? sleepiness_delta * 0.22f : 0.0f;
  const float bored_lid =
      boredom_delta > 0.0f ? boredom_delta * 0.16f : 0.0f;

  micro_behavior_.left_shape.height_scale =
      1.0f + mood_height;
  micro_behavior_.right_shape.height_scale =
      1.0f + mood_height;
  micro_behavior_.left_shape.width_scale =
      1.0f + curiosity_width;
  micro_behavior_.right_shape.width_scale =
      1.0f + curiosity_width;
  micro_behavior_.left_shape.upper_lid =
      clamp01(tired_lid + bored_lid);
  micro_behavior_.right_shape.upper_lid =
      micro_behavior_.left_shape.upper_lid;

  // Deterministic micro gaze. Curiosity and attention raise scanning energy;
  // boredom and sleepiness reduce it. This remains continuous Ghost behavior.
  const float t = static_cast<float>(now_ms - started_ms_) * 0.001f;
  float gaze_energy =
      0.10f +
      (curiosity_delta * 0.18f) +
      (attention_delta * 0.10f) -
      (boredom_delta * 0.10f) -
      (sleepiness_delta * 0.12f);
  if (gaze_energy < 0.02f) gaze_energy = 0.02f;
  if (gaze_energy > 0.18f) gaze_energy = 0.18f;

  micro_behavior_.gaze_x = 0.0f;
  micro_behavior_.gaze_y =
      clampSigned(sinf((t * 0.31f) + 1.2f) * gaze_energy * 0.45f);

  // Heart continuously appears in posture as well as in the face. Keep this
  // small: it is a living posture, not an autonomous gesture.
  const float neck_energy =
      0.10f +
      (curiosity_delta * 0.16f) +
      (attention_delta * 0.10f) -
      (sleepiness_delta * 0.10f) -
      (boredom_delta * 0.08f);
  micro_behavior_.neck_yaw =
      clampSigned(sinf((t * 0.115f) + 0.7f) * neck_energy);
  micro_behavior_.neck_pitch =
      clampSigned(
          (-sleepiness_delta * 0.20f) +
          (attention_delta * 0.08f) +
          (mood_delta * 0.05f) +
          sinf((t * 0.083f) + 2.1f) * 0.025f);

  // Vision events still participate in Behavior arbitration, but immediate
  // sensor-driven body reactions are composed separately by Reflex -> Body Output.
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

  if (!visual_response &&
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
  } else if (!resumed_this_tick && !autonomous_paused_ &&
             autonomous_action_ == AutonomousAction::YAWN &&
             (now_ms - autonomous_action_started_ms_) >= kYawnMs) {
    markAutonomousLifecycle(AutonomousLifecycle::COMPLETE, now_ms);
    autonomous_action_ = AutonomousAction::NONE;
  } else if (!resumed_this_tick && !autonomous_paused_ &&
             autonomous_action_ == AutonomousAction::COFFEE_BREAK &&
             (now_ms - autonomous_action_started_ms_) >= kCoffeeBreakMs) {
    markAutonomousLifecycle(AutonomousLifecycle::COMPLETE, now_ms);
    autonomous_action_ = AutonomousAction::NONE;
  }

  // Immediate sensor-driven body expression is owned by Reflex -> Body Output.
  // Behavior remains the continuous Heart/autonomous stream.

  // Vision interruption still participates in Behavior arbitration, but its
  // immediate directional/light body response is owned by Reflex -> Body Output.

  // Autonomous action layer. It only runs below external event responses.
  // The action is selected from Heart state, never by pure random choice.
  if (!visual_response && !autonomous_paused_) {
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
      micro_behavior_.neck_yaw =
          clampSigned(micro_behavior_.neck_yaw +
                      autonomous_direction_ * 0.55f * envelope);
      micro_behavior_.neck_pitch =
          clampSigned(micro_behavior_.neck_pitch - 0.18f * envelope);

      // High-curiosity looking becomes an animated sparkle because the meaning
      // here is "interested in something", not simply curiosity > threshold.
      if (heart.curiosity >= kCuriousThreshold) {
        micro_behavior_.effect = face::VisualEffect::SPARKLE;
        micro_behavior_.effect_progress = p;
        micro_behavior_.effect_amount =
            clamp01(0.55f + (heart.curiosity - kCuriousThreshold) * 1.5f);
      }
    } else if (autonomous_action_ == AutonomousAction::BORED_SCAN) {
      const float p = clamp01(
          static_cast<float>(autonomous_age) / static_cast<float>(kBoredScanMs));
      const float phase = p * 6.28318531f;
      const float scan_amount = 0.32f + heart.boredom * 0.18f;
      micro_behavior_.gaze_x = 0.0f;
      micro_behavior_.gaze_y = clampSigned(cosf(phase) * 0.05f);
      micro_behavior_.left_shape.upper_lid = 0.05f + heart.boredom * 0.05f;
      micro_behavior_.right_shape.upper_lid =
          micro_behavior_.left_shape.upper_lid;
      micro_behavior_.neck_yaw =
          clampSigned(micro_behavior_.neck_yaw +
                      sinf(phase) * scan_amount * 0.55f);
      micro_behavior_.neck_pitch =
          clampSigned(micro_behavior_.neck_pitch - 0.12f);
    } else if (autonomous_action_ == AutonomousAction::YAWN) {
      const float p = clamp01(
          static_cast<float>(autonomous_age) / static_cast<float>(kYawnMs));
      const float envelope = sinf(p * 3.14159265f);
      micro_behavior_.mouth_open = envelope;
      micro_behavior_.left_shape.upper_lid = 0.10f + 0.35f * envelope;
      micro_behavior_.right_shape.upper_lid =
          micro_behavior_.left_shape.upper_lid;
      resting_openness = clamp01(resting_openness - 0.30f * envelope);
      micro_behavior_.neck_pitch =
          clampSigned(micro_behavior_.neck_pitch + 0.28f * envelope);
    } else if (autonomous_action_ == AutonomousAction::COFFEE_BREAK) {
      const float p = clamp01(
          static_cast<float>(autonomous_age) /
          static_cast<float>(kCoffeeBreakMs));
      const float sip = sinf(p * 3.14159265f);
      micro_behavior_.prop = face::VisualProp::COFFEE_CUP;
      micro_behavior_.prop_progress = p;
      micro_behavior_.mouth_open = 0.10f * sip;
      micro_behavior_.neck_pitch =
          clampSigned(micro_behavior_.neck_pitch - 0.08f * sip);
      micro_behavior_.left_shape.upper_lid = 0.05f + 0.08f * sip;
      micro_behavior_.right_shape.upper_lid =
          micro_behavior_.left_shape.upper_lid;
    }
  }

  // Life-state layer. This persists underneath one-shot autonomous actions.
  if (life_state_ == LifeState::DROWSY) {
    const float drowsy =
        clamp01((heart.sleepiness - kDrowsySleepinessThreshold) / 0.30f);
    resting_openness = clamp01(resting_openness - 0.28f * drowsy);
    const float drowsy_lid = 0.12f * drowsy;
    if (micro_behavior_.left_shape.upper_lid < drowsy_lid) {
      micro_behavior_.left_shape.upper_lid = drowsy_lid;
    }
    micro_behavior_.right_shape.upper_lid =
        micro_behavior_.left_shape.upper_lid;
    micro_behavior_.neck_pitch =
        clampSigned(micro_behavior_.neck_pitch - 0.16f * drowsy);
  } else if (life_state_ == LifeState::SLEEPING) {
    resting_openness = 0.0f;
    micro_behavior_.mouth_open = 0.0f;
    micro_behavior_.prop = face::VisualProp::NONE;
    micro_behavior_.effect = face::VisualEffect::NONE;
    micro_behavior_.effect_progress = 0.0f;
    micro_behavior_.sleep_zzz = true;
    micro_behavior_.sleep_zzz_phase = fmodf(t * 0.18f, 1.0f);
    micro_behavior_.neck_yaw = clampSigned(sinf(t * 0.22f) * 0.05f);
    micro_behavior_.neck_pitch =
        clampSigned(-0.55f + sinf(t * 0.36f) * 0.035f);
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

  // Tiny left/right asymmetry prevents the awake face from looking
  // mechanically mirrored. Sleeping eyes stay fully closed.
  if (life_state_ == LifeState::SLEEPING) {
    micro_behavior_.left_eye_bias = 0.0f;
    micro_behavior_.right_eye_bias = 0.0f;
  } else {
    micro_behavior_.left_eye_bias = sinf((t * 0.23f) + 0.4f) * 0.025f;
    micro_behavior_.right_eye_bias = sinf((t * 0.19f) + 2.0f) * 0.025f;
  }

  micro_behavior_.generated_ms = now_ms;
}

void BehaviorEngine::chooseAutonomousAction(uint32_t now_ms,
                                            const HeartState& heart) {
  last_autonomous_decision_ms_ = now_ms;
  ++autonomous_decision_seq_;
  autonomous_action_started_ms_ = now_ms;

  if (life_state_ == LifeState::SLEEPING) {
    autonomous_action_ = AutonomousAction::NONE;
    ++autonomous_sequence_;
    return;
  }

  if (life_state_ == LifeState::DROWSY) {
    autonomous_action_ =
        (heart.sleepiness >= 0.36f || (autonomous_sequence_ % 2 == 0))
            ? AutonomousAction::YAWN
            : AutonomousAction::COFFEE_BREAK;
    autonomous_paused_ = false;
    markAutonomousLifecycle(AutonomousLifecycle::START, now_ms);
    ++autonomous_sequence_;
    return;
  }

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

void BehaviorEngine::updateLifeState(uint32_t now_ms,
                                     const HeartState& heart) {
  if (static_cast<int32_t>(awake_hold_until_ms_ - now_ms) > 0) {
    life_state_ = LifeState::AWAKE;
    return;
  }

  if (heart.sleepiness >= kSleepSleepinessThreshold &&
      heart.boredom >= kSleepBoredomThreshold) {
    if (life_state_ != LifeState::SLEEPING &&
        autonomous_action_ != AutonomousAction::NONE) {
      markAutonomousLifecycle(AutonomousLifecycle::CANCEL, now_ms);
      autonomous_action_ = AutonomousAction::NONE;
      autonomous_paused_ = false;
    }
    life_state_ = LifeState::SLEEPING;
    return;
  }

  if (heart.sleepiness >= kDrowsySleepinessThreshold &&
      heart.boredom >= kDrowsyBoredomThreshold) {
    life_state_ = LifeState::DROWSY;
    return;
  }

  life_state_ = LifeState::AWAKE;
}

void BehaviorEngine::markAutonomousLifecycle(
    AutonomousLifecycle lifecycle,
    uint32_t now_ms) {
  last_autonomous_lifecycle_ = lifecycle;
  autonomous_lifecycle_ms_ = now_ms;
  ++autonomous_lifecycle_seq_;
}

void BehaviorEngine::triggerEffect(face::VisualEffect effect,
                                   uint32_t now_ms,
                                   uint32_t duration_ms,
                                   float amount) {
  transient_effect_ = effect;
  transient_effect_started_ms_ = now_ms;
  transient_effect_duration_ms_ = duration_ms;
  transient_effect_amount_ = clamp01(amount);
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

#include "HeartEngine.h"
#include <cmath>

namespace deskbot {
namespace ghost {

namespace {

// LOCK 55 first production tuning. These values are deliberately small and
// are implementation tuning, not permanent personality constants.
constexpr uint32_t kBoredomStepIntervalMs = 45000;
constexpr float kBoredomStep = 0.015f;
constexpr uint32_t kSleepinessIdleDelayMs = 90000;
constexpr uint32_t kSleepinessStepIntervalMs = 30000;
constexpr float kSleepinessStep = 0.025f;
constexpr uint32_t kHabituationWindowMs = 12000;
constexpr float kHabituationFloor = 0.20f;
constexpr uint32_t kRecoveryDelayMs = 15000;
constexpr uint32_t kRecoveryStepIntervalMs = 5000;
constexpr float kMoodRecoveryFraction = 0.12f;
constexpr float kCuriosityRecoveryFraction = 0.10f;
constexpr float kAttentionRecoveryFraction = 0.18f;
constexpr float kRecoveryEpsilon = 0.0005f;

}  // namespace

void HeartEngine::begin(uint32_t now_ms) {
  last_appraisal_ = HeartAppraisal{};
  state_ = HeartState{};
  baseline_ = HeartState{};
  primary_snapshot_ = HeartState{};
  restore_plan_ = HeartRestorePlan{};
  primary_snapshot_loaded_ = false;
  first_boot_initialized_ = false;
  primary_save_ok_ = false;
  restore_pending_ = false;

  // LOCK 19 / LOCK 20 / LOCK 51:
  // Legacy Heart-only migration fallback. The coherent Ghost checkpoint is
  // loaded by GhostCore after begin. Without elapsed time, preserve valid legacy
  // values and leave temporal correction pending (new 2026-10-21 LOCK).
  const bool loaded = persistence_.loadPrimary(primary_snapshot_);
  const auto& saved = primary_snapshot_;
  const float values[] = {saved.mood, saved.affection, saved.curiosity,
                         saved.boredom, saved.sleepiness, saved.attention};
  bool valid = loaded;
  for (float value : values)
    valid = valid && std::isfinite(value) && value >= 0 && value <= 1;
  if (valid) {
    primary_snapshot_loaded_ = true;
    state_ = primary_snapshot_; // Unknown elapsed time: preserve, never reset on boot.
    restore_pending_ = true;
  } else {
    first_boot_initialized_ = true;
    primary_save_ok_ = persistence_.savePrimary(state_);
  }

  clampState();
  last_tick_ms_ = now_ms;
  last_heart_change_ms_ = now_ms;
  last_boredom_step_ms_ = now_ms;
  last_sleepiness_step_ms_ = now_ms;
  last_recovery_ms_ = now_ms;
  last_recovery_change_ms_ = 0;
  last_meaningful_stimulus_ms_ = now_ms;
  last_stimulus_ms_ = 0;
  last_stimulus_family_ = 0;
  repeated_stimulus_count_ = 0;
  last_impact_scale_ = 1.0f;
  last_event_type_ = nerve::NeuronType::NONE;
  last_event_ms_ = 0;
}

void HeartEngine::restoreCheckpoint(const HeartState& state,
    const HeartState& baseline, const OfflineElapsed& elapsed, uint32_t now_ms,
    uint32_t quiet_seconds) {
  state_ = state;
  baseline_ = baseline;
  primary_snapshot_ = state;
  primary_snapshot_loaded_ = true;
  first_boot_initialized_ = false;
  restore_pending_ = elapsed.status != ElapsedStatus::KNOWN;
  // Provisional recovery uses the existing online rates after its 15 s delay.
  // No wall clock -> preserve all fields, with elapsed correction pending.
  if (!restore_pending_) {
    const double quiet = static_cast<double>(quiet_seconds);
    const double total = quiet + static_cast<double>(elapsed.seconds);
    const double before = quiet > 15 ? (quiet - 15) / 5.0 : 0.0;
    const double after = total > 15 ? (total - 15) / 5.0 : 0.0;
    const double steps = after - before;
    state_.mood = baseline.mood + (state.mood - baseline.mood) *
        static_cast<float>(std::pow(1.0 - kMoodRecoveryFraction, steps));
    state_.curiosity = baseline.curiosity + (state.curiosity - baseline.curiosity) *
        static_cast<float>(std::pow(1.0 - kCuriosityRecoveryFraction, steps));
    state_.attention = baseline.attention + (state.attention - baseline.attention) *
        static_cast<float>(std::pow(1.0 - kAttentionRecoveryFraction, steps));
    // Fatigue/idle are not clock-of-day estimates. Keep them until rhythm inputs
    // exist; affection and all experience remain unchanged by offline recovery.
  }
  const uint64_t quiet = static_cast<uint64_t>(quiet_seconds) +
      (restore_pending_ ? 0 : elapsed.seconds);
  const uint32_t age_ms = static_cast<uint32_t>(quiet > 86400 ? 86400000 : quiet*1000);
  last_tick_ms_ = last_meaningful_stimulus_ms_ = now_ms;
  last_event_ms_ = now_ms - age_ms;
  last_boredom_step_ms_ = last_sleepiness_step_ms_ = last_recovery_ms_ = now_ms;
  clampState();
}

void HeartEngine::tick(uint32_t now_ms) {
  const uint32_t since_event = now_ms - last_event_ms_;
  const uint32_t since_meaningful =
      now_ms - last_meaningful_stimulus_ms_;

  // LOCK 20-26: temporary Heart values recover toward the current baseline.
  // baseline_ starts from LOCK 28 and is deliberately a separate object so
  // Relationship/long-term learning can move it later without replacing this
  // recovery path.
  if (since_event >= kRecoveryDelayMs &&
      (now_ms - last_recovery_ms_) >= kRecoveryStepIntervalMs) {
    if (applyRecoveryStep(now_ms)) {
      last_heart_change_ms_ = now_ms;
      last_recovery_change_ms_ = now_ms;
    }
    last_recovery_ms_ = now_ms;
  }

  // With no meaningful stimulus, boredom rises slowly. Boredom is contextual,
  // not recovered toward baseline here.
  const uint32_t since_boredom_step = now_ms - last_boredom_step_ms_;
  if (since_meaningful >= kBoredomStepIntervalMs &&
      since_boredom_step >= kBoredomStepIntervalMs) {
    applyDelta(0.0f, 0.0f, 0.0f, kBoredomStep, 0.0f, 0.0f, now_ms);
    last_boredom_step_ms_ = now_ms;
  }

  // LOCK 20 / Step 9 direction: sleepiness has its own time rhythm. Until a
  // wall-clock/day-night rhythm is connected, the first standalone production
  // behavior is inactivity-driven fatigue. This is monotonic TimeEngine time,
  // not a random animation trigger.
  const uint32_t since_sleepiness_step = now_ms - last_sleepiness_step_ms_;
  if (since_meaningful >= kSleepinessIdleDelayMs &&
      since_sleepiness_step >= kSleepinessStepIntervalMs) {
    applyDelta(0.0f, 0.0f, 0.0f, 0.0f, kSleepinessStep, 0.0f, now_ms);
    last_sleepiness_step_ms_ = now_ms;
  }

  last_tick_ms_ = now_ms;
}

void HeartEngine::onNeuron(const nerve::SemanticNeuron& neuron,
                           const HeartContext& event_context,
                           const HeartEventEvidence& evidence) {
  // LOCK 35: appraisal uses the immutable event-entry state and prior evidence.
  last_appraisal_ = HeartAppraisal{};
  if (neuron.type == nerve::NeuronType::TOUCH)
    last_appraisal_ = appraiseTouch(neuron, event_context, evidence);
  last_event_type_ = neuron.type;
  last_event_ms_ = neuron.timestamp_ms;

  const bool meaningful_interaction =
      neuron.type == nerve::NeuronType::TOUCH ||
      neuron.type == nerve::NeuronType::PICKED_UP ||
      neuron.type == nerve::NeuronType::SHAKE ||
      neuron.type == nerve::NeuronType::WAKE_WORD_DETECTED;

  if (meaningful_interaction) {
    last_meaningful_stimulus_ms_ = neuron.timestamp_ms;
    last_boredom_step_ms_ = neuron.timestamp_ms;
    last_sleepiness_step_ms_ = neuron.timestamp_ms;
  }

  const float impact = habituationScaleFor(neuron.type, neuron.timestamp_ms);

  switch (neuron.type) {
    case nerve::NeuronType::TOUCH:
      applyDelta(+0.03f * impact * last_appraisal_.mood_scale,
                 +0.02f * impact * last_appraisal_.affection_scale, 0.0f,
                 -0.04f * impact, -0.04f * impact, +0.04f * impact,
                 neuron.timestamp_ms);
      break;

    case nerve::NeuronType::PICKED_UP:
      applyDelta(0.0f, 0.0f, +0.03f * impact, -0.03f * impact,
                 -0.08f * impact, +0.08f * impact, neuron.timestamp_ms);
      break;

    case nerve::NeuronType::SHAKE:
      applyDelta(-0.06f * impact, 0.0f, 0.0f, -0.02f * impact,
                 -0.12f * impact, +0.10f * impact, neuron.timestamp_ms);
      break;

    case nerve::NeuronType::MOTION_DETECTED: {
      const float strength = clampStrength(neuron.payload.scalar);
      // Ambient low-level vision may attract attention/curiosity, but it must
      // not erase fatigue. Otherwise camera motion can keep DeskRobo awake
      // forever even though Behavior correctly refuses to treat it as a wake
      // stimulus.
      applyDelta(0.0f, 0.0f, +0.02f * strength * impact,
                 -0.01f * strength * impact,
                 0.0f,
                 +0.03f * strength * impact, neuron.timestamp_ms);
      break;
    }

    case nerve::NeuronType::WAKE_WORD_DETECTED:
      applyDelta(0.0f, 0.0f, +0.03f * impact,
                 -0.03f * impact, -0.05f * impact,
                 +0.10f * impact, neuron.timestamp_ms);
      break;

    case nerve::NeuronType::VOICE_ACTIVITY:
      applyDelta(0.0f, 0.0f, +0.01f * impact,
                 0.0f, 0.0f, +0.03f * impact,
                 neuron.timestamp_ms);
      break;

    case nerve::NeuronType::BRIGHTER:
    case nerve::NeuronType::DARKER: {
      const float strength = clampStrength(neuron.payload.scalar);
      applyDelta(0.0f, 0.0f, +0.01f * strength * impact, 0.0f,
                 0.0f,
                 +0.02f * strength * impact, neuron.timestamp_ms);
      break;
    }

    default:
      break;
  }

  clampState();
}

HeartAppraisal HeartEngine::appraiseTouch(
    const nerve::SemanticNeuron& neuron, const HeartContext& context,
    const HeartEventEvidence& evidence) {
  HeartAppraisal result;
  // Provisional bounded reception tuning, not permanent personality learning.
  // Current mood only modulates mood; it is never interpreted as Comfort.
  auto bounded = [](float value, float low, float high) {
    if (!std::isfinite(value)) return 0.0f;
    return value < low ? low : (value > high ? high : value);
  };
  result.mood_scale += bounded(
      context.state.mood - context.baseline.mood, -0.5f, 0.5f) * 0.10f;
  const bool matching = evidence.type == neuron.type &&
                        evidence.person_id == neuron.person_id;
  // Anonymous history is not evidence about the same individual. Do not use it
  // for interpersonal appraisal until recognition supplies an attribution.
  if (matching && neuron.person_id != nerve::kUnknownPerson) {
    if (evidence.has_prior && std::isfinite(evidence.prior_mood_delta) &&
        std::isfinite(evidence.prior_affection_delta)) {
      result.used_prior = true;
      result.mood_scale += bounded(evidence.prior_mood_delta, -0.1f, 0.1f);
      result.affection_scale += bounded(evidence.prior_affection_delta, -0.1f, 0.1f);
    }
    const size_t count = evidence.relationship_experiences;
    if (count > 0 && evidence.positive_experiences <= count &&
        evidence.negative_experiences <= count) {
      result.used_relationship = true;
      // Keep positive and negative evidence distinct: positive reception may
      // strengthen affection, negative reception tempers the mood response.
      result.affection_scale += 0.10f *
          (static_cast<float>(evidence.positive_experiences) / count);
      result.mood_scale -= 0.10f *
          (static_cast<float>(evidence.negative_experiences) / count);
    }
  }
  // No sign inversion or unbounded escalation. Safety/reflex never calls this.
  result.mood_scale = bounded(result.mood_scale, 0.75f, 1.25f);
  result.affection_scale = bounded(result.affection_scale, 0.75f, 1.25f);
  return result;
}

void HeartEngine::onReflexResult(const reflex::ReflexResult& result,
                                 const HeartContext& result_context) {
  // LOCK 47-48: the result of a reflex, including success/failure and whether
  // danger continues, returns to Heart. Exact Heart deltas are intentionally
  // not invented here; this establishes the production feedback boundary.
  (void)result;
  (void)result_context;
  clampState();
}

HeartContext HeartEngine::snapshot(uint32_t now_ms) const {
  HeartContext context;
  context.state = state_;
  context.baseline = baseline_;
  context.captured_ms = now_ms;
  return context;
}

void HeartEngine::setMicroSdBackupHandlers(
    HeartBackupSaveHandler save_handler,
    HeartBackupLoadHandler load_handler,
    void* context) {
  persistence_.setMicroSdBackupHandlers(save_handler, load_handler, context);
}

bool HeartEngine::microSdBackupAvailable() const {
  return persistence_.microSdBackupAvailable();
}

bool HeartEngine::saveMicroSdBackup() const {
  return persistence_.saveBackup(state_);
}

bool HeartEngine::loadMicroSdBackup(HeartState& out_state) const {
  return persistence_.loadBackup(out_state);
}

uint8_t HeartEngine::stimulusFamily(nerve::NeuronType type) {
  switch (type) {
    case nerve::NeuronType::TOUCH:
      return 1;  // touch
    case nerve::NeuronType::PICKED_UP:
    case nerve::NeuronType::SHAKE:
      return 2;  // body motion
    case nerve::NeuronType::MOTION_DETECTED:
    case nerve::NeuronType::BRIGHTER:
    case nerve::NeuronType::DARKER:
      return 3;  // low-level vision
    case nerve::NeuronType::VOICE_ACTIVITY:
    case nerve::NeuronType::WAKE_WORD_DETECTED:
      return 4;  // auditory / call
    default:
      return 0;
  }
}

float HeartEngine::habituationScaleFor(nerve::NeuronType type,
                                       uint32_t now_ms) {
  const uint8_t family = stimulusFamily(type);
  if (family == 0) {
    last_impact_scale_ = 1.0f;
    return last_impact_scale_;
  }

  const bool repeated = last_stimulus_family_ == family &&
                        (now_ms - last_stimulus_ms_) <= kHabituationWindowMs;

  if (repeated) {
    if (repeated_stimulus_count_ < 255) {
      ++repeated_stimulus_count_;
    }
  } else {
    repeated_stimulus_count_ = 0;
  }

  last_stimulus_family_ = family;
  last_stimulus_ms_ = now_ms;

  const float denominator =
      1.0f + 0.8f * static_cast<float>(repeated_stimulus_count_);
  last_impact_scale_ = 1.0f / denominator;
  if (last_impact_scale_ < kHabituationFloor) {
    last_impact_scale_ = kHabituationFloor;
  }
  return last_impact_scale_;
}

bool HeartEngine::applyRecoveryStep(uint32_t now_ms) {
  (void)now_ms;
  bool changed = false;

  const float mood_delta =
      (baseline_.mood - state_.mood) * kMoodRecoveryFraction;
  if (mood_delta > kRecoveryEpsilon || mood_delta < -kRecoveryEpsilon) {
    state_.mood += mood_delta;
    changed = true;
  } else {
    state_.mood = baseline_.mood;
  }

  const float curiosity_delta =
      (baseline_.curiosity - state_.curiosity) * kCuriosityRecoveryFraction;
  if (curiosity_delta > kRecoveryEpsilon ||
      curiosity_delta < -kRecoveryEpsilon) {
    state_.curiosity += curiosity_delta;
    changed = true;
  } else {
    state_.curiosity = baseline_.curiosity;
  }

  const float attention_delta =
      (baseline_.attention - state_.attention) * kAttentionRecoveryFraction;
  if (attention_delta > kRecoveryEpsilon ||
      attention_delta < -kRecoveryEpsilon) {
    state_.attention += attention_delta;
    changed = true;
  } else {
    state_.attention = baseline_.attention;
  }

  // affection is long-term relationship state. sleepiness waits for TimeEngine
  // life rhythm. boredom is recalculated from context/idle time separately.
  clampState();
  return changed;
}

float HeartEngine::clampStrength(float value) {
  if (value <= 0.0f) {
    return 1.0f;
  }
  return clamp01(value);
}

void HeartEngine::applyDelta(float mood,
                             float affection,
                             float curiosity,
                             float boredom,
                             float sleepiness,
                             float attention,
                             uint32_t now_ms) {
  state_.mood += mood;
  state_.affection += affection;
  state_.curiosity += curiosity;
  state_.boredom += boredom;
  state_.sleepiness += sleepiness;
  state_.attention += attention;
  clampState();
  last_heart_change_ms_ = now_ms;
}

float HeartEngine::clamp01(float value) {
  if (value < 0.0f) {
    return 0.0f;
  }
  if (value > 1.0f) {
    return 1.0f;
  }
  return value;
}

void HeartEngine::clampState() {
  state_.mood = clamp01(state_.mood);
  state_.affection = clamp01(state_.affection);
  state_.curiosity = clamp01(state_.curiosity);
  state_.boredom = clamp01(state_.boredom);
  state_.sleepiness = clamp01(state_.sleepiness);
  state_.attention = clamp01(state_.attention);
}

}  // namespace ghost
}  // namespace deskbot

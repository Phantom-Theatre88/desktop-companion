#include "HeartEngine.h"

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
  state_ = HeartState{};
  baseline_ = HeartState{};
  primary_snapshot_ = HeartState{};
  restore_plan_ = HeartRestorePlan{};
  primary_snapshot_loaded_ = false;
  first_boot_initialized_ = false;
  primary_save_ok_ = false;
  restore_pending_ = false;

  // LOCK 19 / LOCK 20 / LOCK 51:
  // NVS is the primary Heart storage. On normal boot, LOCK 20 requires each
  // Heart field to follow a different restoration path. Only affection can be
  // restored exactly from the saved snapshot without inventing missing tuning
  // rules. The other five fields remain explicitly pending their required
  // time/baseline/life-rhythm inputs instead of being mechanically copied.
  if (persistence_.loadPrimary(primary_snapshot_)) {
    primary_snapshot_loaded_ = true;
    state_.affection = primary_snapshot_.affection;
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
                           const HeartContext& event_context) {
  // LOCK 35: use the fixed event-entry snapshot for interpretation. The first
  // production table intentionally covers only senses already proven on the
  // CoreS3. Relationship-dependent interpretation comes later.
  (void)event_context;
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
      applyDelta(+0.03f * impact, +0.02f * impact, 0.0f,
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
      applyDelta(0.0f, 0.0f, +0.02f * strength * impact,
                 -0.01f * strength * impact,
                 -0.03f * strength * impact,
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
                 -0.01f * strength * impact,
                 +0.02f * strength * impact, neuron.timestamp_ms);
      break;
    }

    default:
      break;
  }

  clampState();
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

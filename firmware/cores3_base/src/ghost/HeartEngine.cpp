#include "HeartEngine.h"

namespace deskbot {
namespace ghost {

namespace {

// LOCK 55 first production tuning. These values are deliberately small and
// are implementation tuning, not permanent personality constants.
constexpr uint32_t kBoredomStepIntervalMs = 60000;
constexpr float kBoredomStep = 0.01f;

}  // namespace

void HeartEngine::begin(uint32_t now_ms) {
  state_ = HeartState{};
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
  last_event_type_ = nerve::NeuronType::NONE;
  last_event_ms_ = 0;
}

void HeartEngine::tick(uint32_t now_ms) {
  // LOCK 55: the first time-driven Heart effect is intentionally narrow.
  // With no meaningful stimulus, boredom rises slowly. Recovery toward a
  // dynamic baseline waits for Time/Relationship to provide the inputs locked
  // by LOCK 20-26; do not fake that model here.
  const uint32_t since_event = now_ms - last_event_ms_;
  const uint32_t since_boredom_step = now_ms - last_boredom_step_ms_;
  if (since_event >= kBoredomStepIntervalMs &&
      since_boredom_step >= kBoredomStepIntervalMs) {
    applyDelta(0.0f, 0.0f, 0.0f, kBoredomStep, 0.0f, 0.0f, now_ms);
    last_boredom_step_ms_ = now_ms;
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
  last_boredom_step_ms_ = neuron.timestamp_ms;

  switch (neuron.type) {
    case nerve::NeuronType::TOUCH:
      applyDelta(+0.03f, +0.02f, 0.0f, -0.04f, 0.0f, +0.04f,
                 neuron.timestamp_ms);
      break;

    case nerve::NeuronType::PICKED_UP:
      applyDelta(0.0f, 0.0f, +0.03f, -0.03f, 0.0f, +0.08f,
                 neuron.timestamp_ms);
      break;

    case nerve::NeuronType::SHAKE:
      applyDelta(-0.06f, 0.0f, 0.0f, -0.02f, 0.0f, +0.10f,
                 neuron.timestamp_ms);
      break;

    case nerve::NeuronType::MOTION_DETECTED: {
      const float strength = clampStrength(neuron.payload.scalar);
      applyDelta(0.0f, 0.0f, +0.02f * strength, -0.01f * strength,
                 0.0f, +0.03f * strength, neuron.timestamp_ms);
      break;
    }

    case nerve::NeuronType::BRIGHTER:
    case nerve::NeuronType::DARKER: {
      const float strength = clampStrength(neuron.payload.scalar);
      applyDelta(0.0f, 0.0f, +0.01f * strength, 0.0f,
                 0.0f, +0.02f * strength, neuron.timestamp_ms);
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

#include "HeartEngine.h"

namespace deskbot {
namespace ghost {

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
  last_event_type_ = nerve::NeuronType::NONE;
  last_event_ms_ = 0;
}

void HeartEngine::tick(uint32_t now_ms) {
  // Time is part of Heart from the beginning, but LOCKed design does not yet
  // define concrete decay/recovery coefficients for every field.
  // Keep the production time boundary alive without inventing tuning values.
  last_tick_ms_ = now_ms;
}

void HeartEngine::onNeuron(const nerve::SemanticNeuron& neuron,
                           const HeartContext& event_context) {
  // The event-entry boundary is production code. Exact per-event Heart deltas
  // remain intentionally absent until implementation requires a concrete table.
  // event_context is the fixed snapshot captured at the start of this event.
  (void)event_context;
  last_event_type_ = neuron.type;
  last_event_ms_ = neuron.timestamp_ms;
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

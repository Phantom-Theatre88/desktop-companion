#include "HeartEngine.h"

namespace deskbot {
namespace ghost {

void HeartEngine::begin(uint32_t now_ms) {
  state_ = HeartState{};
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

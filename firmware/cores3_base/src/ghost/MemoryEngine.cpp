#include "MemoryEngine.h"

namespace deskbot {
namespace ghost {

void MemoryEngine::begin(uint32_t now_ms) {
  (void)now_ms;
  short_memory_ = ShortMemory{};
  last_candidate_ = MemoryRecord{};

  for (size_t i = 0; i < kLaneCount; ++i) {
    lane_handlers_[i] = nullptr;
    lane_contexts_[i] = nullptr;
  }
}

void MemoryEngine::onNeuron(const nerve::SemanticNeuron& neuron,
                            const HeartContext& event_context) {
  const bool had_similar_prior_event =
      short_memory_.event_count > 0 && short_memory_.last_type == neuron.type;
  MemoryRecord record;
  record.similar_prior_event = had_similar_prior_event;
  record.kind = MemoryRecordKind::SEMANTIC_EVENT;
  record.neuron = neuron;
  record.heart_context = event_context;
  record.occurred_ms = neuron.timestamp_ms;
  last_candidate_ = record;
  short_memory_.last_type = neuron.type;
  short_memory_.last_source = neuron.source;
  short_memory_.last_event_ms = neuron.timestamp_ms;
  ++short_memory_.event_count;

}

void MemoryEngine::recordNeuronOutcome(const HeartContext& outcome_context) {
  if (last_candidate_.kind != MemoryRecordKind::SEMANTIC_EVENT ||
      last_candidate_.has_outcome) return;
  last_candidate_.outcome_context = outcome_context;
  last_candidate_.has_outcome = true;
}

void MemoryEngine::onReflexResult(const reflex::ReflexResult& result,
                                  const HeartContext& result_context) {
  // LOCK 49: a Reflex result can become memory material without forcing every
  // reflex into permanent storage. Retention/classification policy remains
  // outside this boundary until concrete rules are LOCKed.
  MemoryRecord record;
  record.kind = MemoryRecordKind::REFLEX_RESULT;
  record.reflex_result = result;
  record.heart_context = result_context;
  record.occurred_ms = result.completed_ms;
  last_candidate_ = record;
}

void MemoryEngine::tick(uint32_t now_ms) {
  (void)now_ms;
}

void MemoryEngine::setLaneHandler(MemoryLane lane,
                                  MemoryRecordHandler handler,
                                  void* context) {
  const size_t index = static_cast<size_t>(lane);
  if (index >= kLaneCount) {
    return;
  }

  lane_handlers_[index] = handler;
  lane_contexts_[index] = context;
}

bool MemoryEngine::offerToLane(MemoryLane lane,
                               const MemoryRecord& record) const {
  const size_t index = static_cast<size_t>(lane);
  if (index >= kLaneCount || lane_handlers_[index] == nullptr) {
    return false;
  }

  lane_handlers_[index](record, lane_contexts_[index]);
  return true;
}

reflex::ReflexSensitivityHint MemoryEngine::reflexSensitivityHint(
    const nerve::SemanticNeuron& neuron) const {
  // The production connection exists now, but Step 4 intentionally has no
  // retention/classification rule that can justify a non-baseline modifier yet.
  // Future danger-memory logic can return HEIGHTEN/RELAX here without changing
  // ReflexLayer's public contract.
  reflex::ReflexSensitivityHint hint;
  hint.direction = reflex::ReflexSensitivityDirection::BASELINE;
  hint.stimulus = neuron.type;
  hint.from_experience = false;
  return hint;
}

}  // namespace ghost
}  // namespace deskbot

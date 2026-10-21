#include "MemoryEngine.h"

namespace deskbot {
namespace ghost {

void MemoryEngine::begin(uint32_t now_ms) {
  (void)now_ms;
  short_memory_ = ShortMemory{};
  last_candidate_ = MemoryRecord{};
  experience_count_ = 0;
  next_experience_index_ = 0;
  pending_neuron_ = false;
  for (size_t i = 0; i < kWorkingExperienceCapacity; ++i)
    working_experiences_[i] = MemoryRecord{};

  for (size_t i = 0; i < kLaneCount; ++i) {
    lane_handlers_[i] = nullptr;
    lane_contexts_[i] = nullptr;
  }
}

bool MemoryEngine::recallLatest(nerve::NeuronType type,
                                MemoryRecord& out) const {
  for (size_t i = 0; i < experience_count_; ++i) {
    const size_t index = (next_experience_index_ +
                          kWorkingExperienceCapacity - 1 - i) %
                         kWorkingExperienceCapacity;
    const MemoryRecord& candidate = working_experiences_[index];
    if (candidate.kind == MemoryRecordKind::SEMANTIC_EVENT &&
        candidate.neuron.type == type && candidate.has_outcome) {
      out = candidate;
      return true;
    }
  }
  return false;
}

bool MemoryEngine::recallLatestForPerson(
    nerve::NeuronType type, nerve::PersonId person_id, MemoryRecord& out) const {
  for (size_t i = 0; i < experience_count_; ++i) {
    const size_t index = (next_experience_index_ +
                          kWorkingExperienceCapacity - 1 - i) %
                         kWorkingExperienceCapacity;
    const auto& record = working_experiences_[index];
    if (record.kind == MemoryRecordKind::SEMANTIC_EVENT && record.has_outcome &&
        record.neuron.type == type && record.neuron.person_id == person_id) {
      out = record;
      return true;
    }
  }
  return false;
}

void MemoryEngine::visitPersonExperiences(
    nerve::PersonId person_id, MemoryRecordHandler visitor, void* context) const {
  if (person_id == nerve::kUnknownPerson || visitor == nullptr) return;
  for (size_t i = 0; i < experience_count_; ++i) {
    const size_t index = (next_experience_index_ +
                          kWorkingExperienceCapacity - 1 - i) %
                         kWorkingExperienceCapacity;
    const MemoryRecord& record = working_experiences_[index];
    if (record.kind == MemoryRecordKind::SEMANTIC_EVENT &&
        record.has_outcome && record.neuron.person_id == person_id)
      visitor(record, context);
  }
}

void MemoryEngine::beginNeuronExperience(
    const nerve::SemanticNeuron& neuron,
    const HeartContext& event_context,
    const HeartEventEvidence& evidence) {
  // Pending is intentionally invisible to recallLatest().
  // A new event must not be mistaken for its own prior experience.
  const bool immediate_repeat =
      short_memory_.event_count > 0 && short_memory_.last_type == neuron.type;
  last_candidate_ = MemoryRecord{};
  last_candidate_.kind = MemoryRecordKind::SEMANTIC_EVENT;
  last_candidate_.neuron = neuron;
  last_candidate_.heart_context = event_context;
  last_candidate_.appraisal_evidence = evidence;
  last_candidate_.occurred_ms = neuron.timestamp_ms;
  last_candidate_.similar_prior_event = immediate_repeat;
  pending_neuron_ = true;
}

void MemoryEngine::onNeuron(const nerve::SemanticNeuron& neuron,
                            const HeartContext& event_context) {
  beginNeuronExperience(neuron, event_context);
}

bool MemoryEngine::commitNeuronExperience(
    const HeartContext& outcome_context) {
  if (!pending_neuron_ ||
      last_candidate_.kind != MemoryRecordKind::SEMANTIC_EVENT) {
    return false;
  }
  last_candidate_.outcome_context = outcome_context;
  last_candidate_.has_outcome = true;
  working_experiences_[next_experience_index_] = last_candidate_;
  next_experience_index_ =
      (next_experience_index_ + 1) % kWorkingExperienceCapacity;
  if (experience_count_ < kWorkingExperienceCapacity) ++experience_count_;
  short_memory_.last_type = last_candidate_.neuron.type;
  short_memory_.last_source = last_candidate_.neuron.source;
  short_memory_.last_event_ms = last_candidate_.occurred_ms;
  ++short_memory_.event_count;
  pending_neuron_ = false;
  return true;
}

void MemoryEngine::recordNeuronOutcome(
    const HeartContext& outcome_context) {
  (void)commitNeuronExperience(outcome_context);
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
  // Reflex results are candidates, not neuron experiences. Never allow an
  // asynchronous reflex to silently commit a pending neuron.
  if (!pending_neuron_) last_candidate_ = record;
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

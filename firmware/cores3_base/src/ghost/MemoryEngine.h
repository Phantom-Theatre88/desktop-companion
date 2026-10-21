#pragma once

#include <Arduino.h>
#include "HeartEngine.h"
#include "HeartEventEvidence.h"
#include "../nerve/NerveTypes.h"
#include "../reflex/ReflexLayer.h"

namespace deskbot {
namespace ghost {

struct ShortMemory {
  nerve::NeuronType last_type = nerve::NeuronType::NONE;
  nerve::NeuronSource last_source = nerve::NeuronSource::UNKNOWN;
  uint32_t last_event_ms = 0;
  uint32_t event_count = 0;
};

enum class MemoryRecordKind : uint8_t {
  NONE = 0,
  SEMANTIC_EVENT,
  REFLEX_RESULT,
};

struct MemoryRecord {
  MemoryRecordKind kind = MemoryRecordKind::NONE;
  nerve::SemanticNeuron neuron{};
  reflex::ReflexResult reflex_result{};
  HeartContext heart_context{}; // Event-entry snapshot (LOCK 35)
  HeartContext outcome_context{}; // Heart state after this event
  bool has_outcome = false;
  HeartEventEvidence appraisal_evidence{}; // Prior-only input used this event
  bool similar_prior_event = false; // Legacy immediate-predecessor hint only
  uint32_t occurred_ms = 0;
  uint32_t session_id = 1; // Uptime timestamps are scoped to this boot session.
};

enum class MemoryLane : uint8_t {
  INDIVIDUAL_EXPERIENCE = 0,
  REPEATED_TREND,
  SPECIAL_LONG_TERM,
  RELATIONSHIP_IMPACT,
  COUNT,
};

using MemoryRecordHandler = void (*)(const MemoryRecord& record, void* context);

struct MemorySnapshot {
  uint32_t count = 0;
  uint32_t session_id = 1;
  MemoryRecord records[8]{}; // oldest to newest, committed only
};

class MemoryEngine {
 public:
  void begin(uint32_t now_ms);
  MemorySnapshot exportSnapshot() const;
  void restoreSnapshot(const MemorySnapshot& snapshot);
  void onNeuron(const nerve::SemanticNeuron& neuron,
                const HeartContext& event_context);
  // Read ONLY committed prior experience; an in-flight event is excluded.
  // Exact semantic type is a conservative lookup, not learned similarity.
  bool recallLatest(nerve::NeuronType type, MemoryRecord& out) const;
  // Exact type AND identity, including anonymous-to-anonymous only.
  bool recallLatestForPerson(nerve::NeuronType type, nerve::PersonId person_id,
                             MemoryRecord& out) const;
  // Visit committed person-attributed evidence only, newest first. Pending
  // events and unknown identities are excluded. Callback references are borrowed
  // for this call; consumers must not mutate Memory from the callback.
  void visitPersonExperiences(nerve::PersonId person_id,
                              MemoryRecordHandler visitor, void* context) const;
  // Stage the event without adding it to recallable history.
  void beginNeuronExperience(const nerve::SemanticNeuron& neuron,
                             const HeartContext& event_context,
                             const HeartEventEvidence& evidence = {});
  // Commit one complete experience, preserving both before and after Heart.
  bool commitNeuronExperience(const HeartContext& outcome_context);
  // Compatibility entry point for existing callers: stage only.
  void recordNeuronOutcome(const HeartContext& outcome_context);
  void onReflexResult(const reflex::ReflexResult& result,
                      const HeartContext& result_context);
  void tick(uint32_t now_ms);

  // Step 4 production boundary: retention/classification policy is intentionally
  // kept outside these lanes until concrete rules are LOCKed. The four lanes
  // already exist so later storage engines can be attached without replacing
  // MemoryEngine's public shape.
  void setLaneHandler(MemoryLane lane,
                      MemoryRecordHandler handler,
                      void* context = nullptr);
  bool offerToLane(MemoryLane lane, const MemoryRecord& record) const;

  // LOCK 46 / 50 production boundary. At Step 4 this returns a semantic hint
  // only; Memory does not invent thresholds, gains or decay rates.
  reflex::ReflexSensitivityHint reflexSensitivityHint(
      const nerve::SemanticNeuron& neuron) const;

  const ShortMemory& shortMemory() const { return short_memory_; }
  const MemoryRecord& lastCandidate() const { return last_candidate_; }
  size_t workingExperienceCount() const { return experience_count_; }

 private:
  static constexpr size_t kLaneCount =
      static_cast<size_t>(MemoryLane::COUNT);

  uint32_t session_id_ = 1;
  ShortMemory short_memory_{};
  MemoryRecord last_candidate_{};
  // Bounded working history, now included in Ghost checkpoints. Eight slots
  // are an implementation bound, not a permanent retention/forgetting policy.
  static constexpr size_t kWorkingExperienceCapacity = 8;
  MemoryRecord working_experiences_[kWorkingExperienceCapacity]{};
  size_t experience_count_ = 0;
  size_t next_experience_index_ = 0;
  bool pending_neuron_ = false;
  MemoryRecordHandler lane_handlers_[kLaneCount]{};
  void* lane_contexts_[kLaneCount]{};
};

}  // namespace ghost
}  // namespace deskbot

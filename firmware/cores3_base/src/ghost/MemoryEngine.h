#pragma once

#include <Arduino.h>
#include "HeartEngine.h"
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
  HeartContext heart_context{};
  uint32_t occurred_ms = 0;
};

enum class MemoryLane : uint8_t {
  INDIVIDUAL_EXPERIENCE = 0,
  REPEATED_TREND,
  SPECIAL_LONG_TERM,
  RELATIONSHIP_IMPACT,
  COUNT,
};

using MemoryRecordHandler = void (*)(const MemoryRecord& record, void* context);

class MemoryEngine {
 public:
  void begin(uint32_t now_ms);
  void onNeuron(const nerve::SemanticNeuron& neuron,
                const HeartContext& event_context);
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

 private:
  static constexpr size_t kLaneCount =
      static_cast<size_t>(MemoryLane::COUNT);

  ShortMemory short_memory_{};
  MemoryRecord last_candidate_{};
  MemoryRecordHandler lane_handlers_[kLaneCount]{};
  void* lane_contexts_[kLaneCount]{};
};

}  // namespace ghost
}  // namespace deskbot

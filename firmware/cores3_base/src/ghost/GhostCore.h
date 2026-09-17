#pragma once

#include <Arduino.h>
#include "BehaviorEngine.h"
#include "HeartEngine.h"
#include "MemoryEngine.h"
#include "RelationshipEngine.h"
#include "TimeEngine.h"
#include "../nerve/NerveTypes.h"
#include "../reflex/ReflexLayer.h"

namespace deskbot {
namespace ghost {

class GhostCore {
 public:
  void begin(uint32_t now_ms);
  void onNeuron(const nerve::SemanticNeuron& neuron);
  void onReflexResult(const reflex::ReflexResult& result);
  void tick(uint32_t now_ms);

  const HeartState& heart() const { return heart_.state(); }
  HeartContext heartContext(uint32_t now_ms) const { return heart_.snapshot(now_ms); }
  const ShortMemory& memory() const { return memory_.shortMemory(); }

  HeartEngine& heartEngine() { return heart_; }
  MemoryEngine& memoryEngine() { return memory_; }
  TimeEngine& timeEngine() { return time_; }
  RelationshipEngine& relationshipEngine() { return relationship_; }
  BehaviorEngine& behaviorEngine() { return behavior_; }

  static void synapseHandler(const nerve::SemanticNeuron& neuron, void* context);

 private:
  HeartEngine heart_{};
  MemoryEngine memory_{};
  TimeEngine time_{};
  RelationshipEngine relationship_{};
  BehaviorEngine behavior_{};
};

}  // namespace ghost
}  // namespace deskbot

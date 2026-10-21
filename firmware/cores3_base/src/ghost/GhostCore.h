#pragma once

#include <Arduino.h>
#include "BehaviorEngine.h"
#include "HeartEngine.h"
#include "MemoryEngine.h"
#include "RelationshipEngine.h"
#include "TimeEngine.h"
#include "GhostPersistence.h"
#include "../nerve/NerveTypes.h"
#include "../reflex/ReflexLayer.h"

namespace deskbot {
namespace ghost {

class GhostCore {
 public:
  void configurePersistence(CheckpointStore primary, CheckpointStore backup = {}) {
    persistence_.configure(primary, backup);
  }
  bool saveCheckpoint();
  const GhostPersistence& persistence() const { return persistence_; }
  ElapsedStatus offlineTimeStatus() const { return offline_status_; }
  void begin(uint32_t now_ms);
  void onNeuron(const nerve::SemanticNeuron& neuron);
  void onReflexResult(const reflex::ReflexResult& result);
  void tick(uint32_t now_ms);

  const HeartState& heart() const { return heart_.state(); }
  HeartContext heartContext(uint32_t now_ms) const { return heart_.snapshot(now_ms); }
  const ShortMemory& memory() const { return memory_.shortMemory(); }
  nerve::NeuronType lastNeuronType() const { return last_neuron_type_; }
  uint32_t lastNeuronMs() const { return last_neuron_ms_; }

  reflex::ReflexSensitivityHint reflexSensitivityHint(
      const nerve::SemanticNeuron& neuron) const {
    return memory_.reflexSensitivityHint(neuron);
  }

  HeartEngine& heartEngine() { return heart_; }
  MemoryEngine& memoryEngine() { return memory_; }
  TimeEngine& timeEngine() { return time_; }
  RelationshipEngine& relationshipEngine() { return relationship_; }
  BehaviorEngine& behaviorEngine() { return behavior_; }

  static void synapseHandler(const nerve::SemanticNeuron& neuron, void* context);

 private:
  GhostPersistence persistence_{};
  GhostCheckpoint checkpoint_{};
  uint32_t last_checkpoint_attempt_ms_ = 0;
  uint32_t observed_heart_change_ms_ = 0;
  bool checkpoint_dirty_ = true;
  ElapsedStatus offline_status_ = ElapsedStatus::CLOCK_UNKNOWN;
  HeartEngine heart_{};
  MemoryEngine memory_{};
  TimeEngine time_{};
  RelationshipEngine relationship_{};
  BehaviorEngine behavior_{};
  nerve::NeuronType last_neuron_type_ = nerve::NeuronType::NONE;
  uint32_t last_neuron_ms_ = 0;
};

}  // namespace ghost
}  // namespace deskbot

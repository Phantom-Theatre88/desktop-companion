#pragma once

#include <Arduino.h>
#include "../ghost/GhostCore.h"
#include "../nerve/SynapseRouter.h"
#include "../reflex/ReflexLayer.h"

namespace deskbot {
namespace core {

class DesktopCompanionRuntime {
 public:
  bool begin(uint32_t now_ms);
  void tick(uint32_t now_ms);
  void emit(const nerve::SemanticNeuron& neuron);
  void reportReflexResult(const reflex::ReflexResult& result);

  nerve::SynapseRouter& synapse() { return synapse_; }
  ghost::GhostCore& ghost() { return ghost_; }
  reflex::ReflexLayer& reflex() { return reflex_; }

 private:
  bool connectGhost(nerve::NeuronType type);
  bool connectReflex(nerve::NeuronType type);

  nerve::SynapseRouter synapse_{};
  ghost::GhostCore ghost_{};
  reflex::ReflexLayer reflex_{};
};

}  // namespace core
}  // namespace deskbot

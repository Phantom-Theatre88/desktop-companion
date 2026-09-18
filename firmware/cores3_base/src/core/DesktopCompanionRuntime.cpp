#include "DesktopCompanionRuntime.h"

namespace deskbot {
namespace core {

bool DesktopCompanionRuntime::begin(uint32_t now_ms) {
  ghost_.begin(now_ms);

  // LOCK 46 / 50: Reflex may consult Memory/Ghost before selecting a response.
  // Step 4 wires the boundary only; numeric sensitivity changes stay unimplemented
  // until concrete thresholds/gains/time constants are LOCKed.
  reflex_.setSensitivityProvider(
      [](const nerve::SemanticNeuron& neuron,
         void* context) -> reflex::ReflexSensitivityHint {
        auto* self = static_cast<DesktopCompanionRuntime*>(context);
        if (self == nullptr) {
          return reflex::ReflexSensitivityHint{};
        }
        return self->ghost_.reflexSensitivityHint(neuron);
      },
      this);

  bool ok = true;

  const nerve::NeuronType ghost_inputs[] = {
      nerve::NeuronType::PROXIMITY_NEAR,
      nerve::NeuronType::PROXIMITY_APPROACHING,
      nerve::NeuronType::PROXIMITY_LEAVING,
      nerve::NeuronType::PERSON_PRESENT,
      nerve::NeuronType::PERSON_ABSENT,
      nerve::NeuronType::TOUCH,
      nerve::NeuronType::STROKE_DETECTED,
      nerve::NeuronType::PICKED_UP,
      nerve::NeuronType::SHAKE,
      nerve::NeuronType::FACE_DETECTED,
      nerve::NeuronType::FACE_LOST,
      nerve::NeuronType::LOUD_SOUND,
      nerve::NeuronType::VOICE_ACTIVITY,
      nerve::NeuronType::MOTION_DETECTED,
      nerve::NeuronType::BRIGHTER,
      nerve::NeuronType::DARKER,
  };

  for (const auto type : ghost_inputs) {
    ok = connectGhost(type) && ok;
  }

  const nerve::NeuronType reflex_inputs[] = {
      nerve::NeuronType::PROXIMITY_NEAR,
      nerve::NeuronType::PROXIMITY_APPROACHING,
      nerve::NeuronType::PERSON_PRESENT,
      nerve::NeuronType::FACE_DETECTED,
      nerve::NeuronType::LOUD_SOUND,
      nerve::NeuronType::PICKED_UP,
      nerve::NeuronType::SHAKE,
  };

  for (const auto type : reflex_inputs) {
    ok = connectReflex(type) && ok;
  }

  return ok;
}

void DesktopCompanionRuntime::tick(uint32_t now_ms) {
  ghost_.tick(now_ms);
}

void DesktopCompanionRuntime::emit(const nerve::SemanticNeuron& neuron) {
  synapse_.emit(neuron);
}

void DesktopCompanionRuntime::reportReflexResult(const reflex::ReflexResult& result) {
  ghost_.onReflexResult(result);
}

bool DesktopCompanionRuntime::connectGhost(nerve::NeuronType type) {
  return synapse_.connect(
      type,
      nerve::SynapseTarget::GHOST,
      ghost::GhostCore::synapseHandler,
      &ghost_);
}

bool DesktopCompanionRuntime::connectReflex(nerve::NeuronType type) {
  return synapse_.connect(
      type,
      nerve::SynapseTarget::REFLEX,
      reflex::ReflexLayer::synapseHandler,
      &reflex_);
}

}  // namespace core
}  // namespace deskbot

#include "ReflexLayer.h"

namespace deskbot {
namespace reflex {

void ReflexLayer::setIntentHandler(ReflexIntentHandler handler, void* context) {
  handler_ = handler;
  handler_context_ = context;
}

void ReflexLayer::setSensitivityProvider(ReflexSensitivityProvider provider,
                                         void* context) {
  sensitivity_provider_ = provider;
  sensitivity_context_ = context;
}

void ReflexLayer::onNeuron(const nerve::SemanticNeuron& neuron) {
  // LOCK 46 / 50: Memory/Ghost may provide an experience-derived sensitivity
  // hint before Reflex chooses a response. The hint is intentionally not mapped
  // to concrete thresholds/strength/timing yet because those values are not LOCKed.
  last_sensitivity_hint_ = ReflexSensitivityHint{};
  if (sensitivity_provider_ != nullptr) {
    last_sensitivity_hint_ = sensitivity_provider_(neuron, sensitivity_context_);
  }

  switch (neuron.type) {
    case nerve::NeuronType::PROXIMITY_NEAR:
    case nerve::NeuronType::PROXIMITY_APPROACHING:
    case nerve::NeuronType::PERSON_PRESENT:
    case nerve::NeuronType::FACE_DETECTED:
    case nerve::NeuronType::LOUD_SOUND:
    case nerve::NeuronType::WAKE_WORD_DETECTED:
    case nerve::NeuronType::MOTION_DETECTED:
      emit(ReflexIntentType::LOOK_TOWARD_SOURCE, neuron);
      break;

    case nerve::NeuronType::TOUCH:
      emit(ReflexIntentType::TOUCH_RESPONSE, neuron);
      break;

    case nerve::NeuronType::LIFT_STARTED:
      emit(ReflexIntentType::STARTLE, neuron);
      break;

    case nerve::NeuronType::SHAKE:
      emit(ReflexIntentType::SHAKE_RESPONSE, neuron);
      break;

    case nerve::NeuronType::BRIGHTER:
    case nerve::NeuronType::DARKER:
      emit(ReflexIntentType::LIGHT_ADAPT, neuron);
      break;

    default:
      break;
  }
}

void ReflexLayer::synapseHandler(const nerve::SemanticNeuron& neuron, void* context) {
  auto* self = static_cast<ReflexLayer*>(context);
  if (self != nullptr) {
    self->onNeuron(neuron);
  }
}

void ReflexLayer::emit(ReflexIntentType type, const nerve::SemanticNeuron& cause) {
  if (handler_ == nullptr) {
    return;
  }

  ReflexIntent intent;
  intent.type = type;
  intent.created_ms = cause.timestamp_ms;
  intent.cause = cause.type;
  intent.payload = cause.payload;
  intent.confidence = cause.confidence;
  handler_(intent, handler_context_);
}

}  // namespace reflex
}  // namespace deskbot

#include "ReflexLayer.h"

namespace deskbot {
namespace reflex {

void ReflexLayer::setIntentHandler(ReflexIntentHandler handler, void* context) {
  handler_ = handler;
  handler_context_ = context;
}

void ReflexLayer::onNeuron(const nerve::SemanticNeuron& neuron) {
  switch (neuron.type) {
    case nerve::NeuronType::PROXIMITY_NEAR:
    case nerve::NeuronType::PROXIMITY_APPROACHING:
    case nerve::NeuronType::PERSON_PRESENT:
    case nerve::NeuronType::FACE_DETECTED:
    case nerve::NeuronType::LOUD_SOUND:
      emit(ReflexIntentType::LOOK_TOWARD_SOURCE, neuron);
      break;

    case nerve::NeuronType::PICKED_UP:
    case nerve::NeuronType::SHAKE:
      emit(ReflexIntentType::WIDEN_EYES, neuron);
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
  handler_(intent, handler_context_);
}

}  // namespace reflex
}  // namespace deskbot

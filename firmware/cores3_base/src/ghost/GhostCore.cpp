#include "GhostCore.h"

namespace deskbot {
namespace ghost {

void GhostCore::begin(uint32_t now_ms) {
  heart_.begin(now_ms);
  memory_.begin(now_ms);
  time_.begin(now_ms);
  relationship_.begin(now_ms);
  behavior_.begin(now_ms);
}

void GhostCore::onNeuron(const nerve::SemanticNeuron& neuron) {
  // LOCK 35: capture Heart Context once at the start of this event and use
  // the same read-only snapshot throughout the event processing unit.
  const HeartContext event_context = heart_.snapshot(neuron.timestamp_ms);

  heart_.onNeuron(neuron, event_context);
  memory_.onNeuron(neuron, event_context);
  relationship_.onNeuron(neuron, event_context);
  behavior_.onNeuron(neuron, event_context);
}

void GhostCore::onReflexResult(const reflex::ReflexResult& result) {
  // LOCK 47-49: the completed reflex result returns to Ghost and is offered to
  // Heart and Memory using one fixed Heart Context snapshot for this result.
  const HeartContext result_context = heart_.snapshot(result.completed_ms);
  heart_.onReflexResult(result, result_context);
  memory_.onReflexResult(result, result_context);
}

void GhostCore::tick(uint32_t now_ms) {
  time_.tick(now_ms);
  heart_.tick(now_ms);
  memory_.tick(now_ms);
  relationship_.tick(now_ms);
  behavior_.tick(now_ms);
}

void GhostCore::synapseHandler(const nerve::SemanticNeuron& neuron, void* context) {
  auto* self = static_cast<GhostCore*>(context);
  if (self != nullptr) {
    self->onNeuron(neuron);
  }
}

}  // namespace ghost
}  // namespace deskbot

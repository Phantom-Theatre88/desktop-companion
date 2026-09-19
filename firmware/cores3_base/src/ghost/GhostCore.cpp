#include "GhostCore.h"

namespace deskbot {
namespace ghost {

void GhostCore::begin(uint32_t now_ms) {
  last_neuron_type_ = nerve::NeuronType::NONE;
  last_neuron_ms_ = 0;
  heart_.begin(now_ms);
  memory_.begin(now_ms);
  time_.begin(now_ms);
  relationship_.begin(now_ms);
  behavior_.begin(now_ms);
}

void GhostCore::onNeuron(const nerve::SemanticNeuron& neuron) {
  last_neuron_type_ = neuron.type;
  last_neuron_ms_ = neuron.timestamp_ms;
  // LOCK 35: capture Heart Context once at the start of this event and use
  // the same read-only snapshot throughout the event processing unit.
  // SemanticNeuron::timestamp_ms remains the occurrence time. captured_ms is
  // the Ghost handling-time clock supplied by TimeEngine, which Behavior uses
  // for arbitration/lifetimes so blocking device work cannot age an action.
  const HeartContext event_context = heart_.snapshot(time_.lastTickMs());

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

  // LOCK 52: standalone DeskRobo life must exist before external sensors.
  // Behavior receives the current read-only Heart snapshot every runtime tick
  // and produces continuous micro-behavior for the body/face layer.
  behavior_.tick(now_ms, heart_.snapshot(now_ms));
}

void GhostCore::synapseHandler(const nerve::SemanticNeuron& neuron, void* context) {
  auto* self = static_cast<GhostCore*>(context);
  if (self != nullptr) {
    self->onNeuron(neuron);
  }
}

}  // namespace ghost
}  // namespace deskbot

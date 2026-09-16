#include "GhostCore.h"

namespace deskbot {
namespace ghost {

void GhostCore::begin(uint32_t now_ms) {
  last_tick_ms_ = now_ms;
}

void GhostCore::onNeuron(const nerve::SemanticNeuron& neuron) {
  memory_.last_type = neuron.type;
  memory_.last_source = neuron.source;
  memory_.last_event_ms = neuron.timestamp_ms;
  ++memory_.event_count;

  // Heart numeric rules are intentionally not hard-coded here yet.
  // The sacred docs still mark detailed Heart tuning as undecided.
  // This ingress is production wiring: semantic events now reach Ghost.
}

void GhostCore::tick(uint32_t now_ms) {
  // Time enters Ghost from the first implementation, but detailed state
  // transition rates are added only after the Heart numeric spec is LOCKed.
  last_tick_ms_ = now_ms;
}

void GhostCore::synapseHandler(const nerve::SemanticNeuron& neuron, void* context) {
  auto* self = static_cast<GhostCore*>(context);
  if (self != nullptr) {
    self->onNeuron(neuron);
  }
}

}  // namespace ghost
}  // namespace deskbot

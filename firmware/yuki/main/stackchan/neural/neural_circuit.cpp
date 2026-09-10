#include "stackchan/neural/neural_circuit.h"

namespace yuki::neural {

bool NeuralCircuit::connect(SemanticEventId event_id, SynapseCallback callback, void* context) {
    if (callback == nullptr || event_id == SemanticEventId::kUnknown) {
        return false;
    }

    for (auto& synapse : synapses_) {
        if (!synapse.enabled) {
            synapse.event_id = event_id;
            synapse.callback = callback;
            synapse.context = context;
            synapse.enabled = true;
            return true;
        }
    }

    return false;
}

void NeuralCircuit::disconnect_all() {
    for (auto& synapse : synapses_) {
        synapse = {};
    }
}

void NeuralCircuit::emit(const SemanticEvent& event) const {
    if (event.id == SemanticEventId::kUnknown) {
        return;
    }

    for (const auto& synapse : synapses_) {
        if (synapse.enabled && synapse.event_id == event.id && synapse.callback != nullptr) {
            synapse.callback(event, synapse.context);
        }
    }
}

std::size_t NeuralCircuit::connection_count() const {
    std::size_t count = 0;
    for (const auto& synapse : synapses_) {
        if (synapse.enabled) {
            ++count;
        }
    }
    return count;
}

}  // namespace yuki::neural

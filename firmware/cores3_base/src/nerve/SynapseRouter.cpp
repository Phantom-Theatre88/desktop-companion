#include "SynapseRouter.h"

namespace deskbot {
namespace nerve {

bool SynapseRouter::connect(NeuronType type,
                            SynapseTarget target,
                            SynapseHandler handler,
                            void* context) {
  if (handler == nullptr || binding_count_ >= kMaxBindings) {
    return false;
  }

  bindings_[binding_count_++] = SynapseBinding{type, target, handler, context};
  return true;
}

void SynapseRouter::emit(const SemanticNeuron& neuron) const {
  for (size_t i = 0; i < binding_count_; ++i) {
    const auto& binding = bindings_[i];
    if (binding.type == neuron.type && binding.handler != nullptr) {
      binding.handler(neuron, binding.context);
    }
  }
}

}  // namespace nerve
}  // namespace deskbot

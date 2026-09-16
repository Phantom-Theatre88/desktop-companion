#pragma once

#include "NerveTypes.h"

namespace deskbot {
namespace nerve {

enum class SynapseTarget : uint8_t {
  REFLEX = 0,
  GHOST,
  MEMORY,
  PI5,
  BEHAVIOR,
};

using SynapseHandler = void (*)(const SemanticNeuron& neuron, void* context);

struct SynapseBinding {
  NeuronType type = NeuronType::NONE;
  SynapseTarget target = SynapseTarget::GHOST;
  SynapseHandler handler = nullptr;
  void* context = nullptr;
};

class SynapseRouter {
 public:
  static constexpr size_t kMaxBindings = 24;

  bool connect(NeuronType type,
               SynapseTarget target,
               SynapseHandler handler,
               void* context = nullptr);

  void emit(const SemanticNeuron& neuron) const;
  size_t bindingCount() const { return binding_count_; }

 private:
  SynapseBinding bindings_[kMaxBindings]{};
  size_t binding_count_ = 0;
};

}  // namespace nerve
}  // namespace deskbot

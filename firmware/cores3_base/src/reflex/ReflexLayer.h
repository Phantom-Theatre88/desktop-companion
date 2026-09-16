#pragma once

#include <Arduino.h>
#include "../nerve/NerveTypes.h"

namespace deskbot {
namespace reflex {

enum class ReflexIntentType : uint8_t {
  NONE = 0,
  LOOK_TOWARD_SOURCE,
  WIDEN_EYES,
  HOLD_STILL,
};

struct ReflexIntent {
  ReflexIntentType type = ReflexIntentType::NONE;
  uint32_t created_ms = 0;
  nerve::NeuronType cause = nerve::NeuronType::NONE;
};

using ReflexIntentHandler = void (*)(const ReflexIntent& intent, void* context);

class ReflexLayer {
 public:
  void setIntentHandler(ReflexIntentHandler handler, void* context = nullptr);
  void onNeuron(const nerve::SemanticNeuron& neuron);

  static void synapseHandler(const nerve::SemanticNeuron& neuron, void* context);

 private:
  void emit(ReflexIntentType type, const nerve::SemanticNeuron& cause);

  ReflexIntentHandler handler_ = nullptr;
  void* handler_context_ = nullptr;
};

}  // namespace reflex
}  // namespace deskbot

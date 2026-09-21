#pragma once

#include <Arduino.h>
#include "../nerve/NerveTypes.h"

namespace deskbot {
namespace reflex {

enum class ReflexIntentType : uint8_t {
  NONE = 0,
  LOOK_TOWARD_SOURCE,
  WIDEN_EYES,
  TOUCH_RESPONSE,
  STARTLE,
  SHAKE_RESPONSE,
  LIGHT_ADAPT,
  HOLD_STILL,
};

struct ReflexIntent {
  ReflexIntentType type = ReflexIntentType::NONE;
  uint32_t created_ms = 0;
  nerve::NeuronType cause = nerve::NeuronType::NONE;
  nerve::NeuronPayload payload{};
  float confidence = 1.0f;
};

enum class ReflexOutcome : uint8_t {
  UNKNOWN = 0,
  SUCCEEDED,
  FAILED,
};

struct ReflexResult {
  ReflexIntent intent{};
  ReflexOutcome outcome = ReflexOutcome::UNKNOWN;
  bool danger_continues = false;
  uint32_t completed_ms = 0;
};

// Step 4 production boundary for LOCK 46 / 50.
// The direction can be supplied by Memory/Ghost without fixing numeric
// thresholds, gains or time constants yet.
enum class ReflexSensitivityDirection : uint8_t {
  BASELINE = 0,
  HEIGHTEN,
  RELAX,
};

struct ReflexSensitivityHint {
  ReflexSensitivityDirection direction = ReflexSensitivityDirection::BASELINE;
  nerve::NeuronType stimulus = nerve::NeuronType::NONE;
  bool from_experience = false;
};

using ReflexIntentHandler = void (*)(const ReflexIntent& intent, void* context);
using ReflexSensitivityProvider = ReflexSensitivityHint (*)(
    const nerve::SemanticNeuron& neuron,
    void* context);

class ReflexLayer {
 public:
  void setIntentHandler(ReflexIntentHandler handler, void* context = nullptr);
  void setSensitivityProvider(ReflexSensitivityProvider provider,
                              void* context = nullptr);
  void onNeuron(const nerve::SemanticNeuron& neuron);

  const ReflexSensitivityHint& lastSensitivityHint() const {
    return last_sensitivity_hint_;
  }

  static void synapseHandler(const nerve::SemanticNeuron& neuron, void* context);

 private:
  void emit(ReflexIntentType type, const nerve::SemanticNeuron& cause);

  ReflexIntentHandler handler_ = nullptr;
  void* handler_context_ = nullptr;
  ReflexSensitivityProvider sensitivity_provider_ = nullptr;
  void* sensitivity_context_ = nullptr;
  ReflexSensitivityHint last_sensitivity_hint_{};
};

}  // namespace reflex
}  // namespace deskbot

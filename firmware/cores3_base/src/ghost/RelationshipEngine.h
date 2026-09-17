#pragma once

#include <Arduino.h>
#include "HeartEngine.h"
#include "../nerve/NerveTypes.h"

namespace deskbot {
namespace ghost {

class RelationshipEngine {
 public:
  void begin(uint32_t now_ms) { (void)now_ms; }

  void onNeuron(const nerve::SemanticNeuron& neuron,
                const HeartContext& event_context) {
    (void)neuron;
    (void)event_context;
  }

  void tick(uint32_t now_ms) { (void)now_ms; }
};

}  // namespace ghost
}  // namespace deskbot

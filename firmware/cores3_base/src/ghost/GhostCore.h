#pragma once

#include <Arduino.h>
#include "../nerve/NerveTypes.h"

namespace deskbot {
namespace ghost {

struct HeartState {
  float mood = 0.5f;
  float affection = 0.5f;
  float curiosity = 0.5f;
  float boredom = 0.0f;
  float sleepiness = 0.0f;
  float attention = 0.0f;
};

struct ShortMemory {
  nerve::NeuronType last_type = nerve::NeuronType::NONE;
  nerve::NeuronSource last_source = nerve::NeuronSource::UNKNOWN;
  uint32_t last_event_ms = 0;
  uint32_t event_count = 0;
};

class GhostCore {
 public:
  void begin(uint32_t now_ms);
  void onNeuron(const nerve::SemanticNeuron& neuron);
  void tick(uint32_t now_ms);

  const HeartState& heart() const { return heart_; }
  const ShortMemory& memory() const { return memory_; }

  static void synapseHandler(const nerve::SemanticNeuron& neuron, void* context);

 private:
  HeartState heart_{};
  ShortMemory memory_{};
  uint32_t last_tick_ms_ = 0;
};

}  // namespace ghost
}  // namespace deskbot

#pragma once

#include <Arduino.h>
#include "HeartEngine.h"
#include "../nerve/NerveTypes.h"

namespace deskbot {
namespace ghost {

struct ShortMemory {
  nerve::NeuronType last_type = nerve::NeuronType::NONE;
  nerve::NeuronSource last_source = nerve::NeuronSource::UNKNOWN;
  uint32_t last_event_ms = 0;
  uint32_t event_count = 0;
};

class MemoryEngine {
 public:
  void begin(uint32_t now_ms) {
    (void)now_ms;
    short_memory_ = ShortMemory{};
  }

  void onNeuron(const nerve::SemanticNeuron& neuron,
                const HeartContext& event_context) {
    (void)event_context;
    short_memory_.last_type = neuron.type;
    short_memory_.last_source = neuron.source;
    short_memory_.last_event_ms = neuron.timestamp_ms;
    ++short_memory_.event_count;
  }

  void tick(uint32_t now_ms) { (void)now_ms; }

  const ShortMemory& shortMemory() const { return short_memory_; }

 private:
  ShortMemory short_memory_{};
};

}  // namespace ghost
}  // namespace deskbot

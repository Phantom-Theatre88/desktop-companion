#pragma once

#include <Arduino.h>
#include "HeartEngine.h"
#include "../nerve/NerveTypes.h"

namespace deskbot {
namespace ghost {

struct MicroBehaviorFrame {
  float eye_openness = 0.90f;
  float gaze_x = 0.0f;
  float gaze_y = 0.0f;
  float left_eye_bias = 0.0f;
  float right_eye_bias = 0.0f;
  bool blink_active = false;
  uint32_t generated_ms = 0;
};

class BehaviorEngine {
 public:
  void begin(uint32_t now_ms);

  void onNeuron(const nerve::SemanticNeuron& neuron,
                const HeartContext& event_context);

  void tick(uint32_t now_ms, const HeartContext& heart_context);

  const MicroBehaviorFrame& microBehavior() const { return micro_behavior_; }

 private:
  static float clamp01(float value);
  static float clampSigned(float value);

  uint32_t started_ms_ = 0;
  uint32_t last_event_ms_ = 0;
  nerve::NeuronType last_event_type_ = nerve::NeuronType::NONE;
  MicroBehaviorFrame micro_behavior_{};
};

}  // namespace ghost
}  // namespace deskbot

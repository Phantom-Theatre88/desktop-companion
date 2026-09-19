#pragma once

#include <Arduino.h>
#include "HeartEngine.h"
#include "../face/FaceShape.h"
#include "../nerve/NerveTypes.h"

namespace deskbot {
namespace ghost {

enum class AutonomousAction : uint8_t {
  NONE = 0,
  CURIOUS_LOOK,
  BORED_SCAN,
};

enum class AutonomousLifecycle : uint8_t {
  NONE = 0,
  START,
  PAUSE,
  RESUME,
  COMPLETE,
  CANCEL,
};

struct MicroBehaviorFrame {
  float eye_openness = 0.90f;
  float gaze_x = 0.0f;
  float gaze_y = 0.0f;
  float left_eye_bias = 0.0f;
  float right_eye_bias = 0.0f;
  face::EyeShape left_shape{}, right_shape{};
  float eye_spacing_scale = 1.0f;
  float jitter_x = 0.0f, jitter_y = 0.0f;
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
  nerve::NeuronType lastReceivedType() const { return last_received_type_; }
  uint32_t lastReceivedMs() const { return last_received_ms_; }
  AutonomousAction autonomousAction() const { return autonomous_action_; }
  uint32_t lastAutonomousDecisionMs() const { return last_autonomous_decision_ms_; }
  uint32_t autonomousDecisionSeq() const { return autonomous_decision_seq_; }
  bool autonomousPaused() const { return autonomous_paused_; }
  AutonomousLifecycle lastAutonomousLifecycle() const { return last_autonomous_lifecycle_; }
  uint32_t autonomousLifecycleSeq() const { return autonomous_lifecycle_seq_; }

 private:
  static float clamp01(float value);
  static float clampSigned(float value);
  void chooseAutonomousAction(uint32_t now_ms, const HeartState& heart);
  void markAutonomousLifecycle(AutonomousLifecycle lifecycle, uint32_t now_ms);

  nerve::NeuronType last_received_type_ = nerve::NeuronType::NONE;
  uint32_t last_received_ms_ = 0;
  nerve::NeuronType visual_event_type_ = nerve::NeuronType::NONE;
  uint32_t visual_event_ms_ = 0;
  uint32_t started_ms_ = 0;
  uint32_t last_event_ms_ = 0;
  nerve::NeuronType last_event_type_ = nerve::NeuronType::NONE;
  AutonomousAction autonomous_action_ = AutonomousAction::NONE;
  uint32_t autonomous_action_started_ms_ = 0;
  uint32_t last_autonomous_decision_ms_ = 0;
  uint32_t autonomous_decision_seq_ = 0;
  uint8_t autonomous_sequence_ = 0;
  float autonomous_direction_ = 1.0f;
  bool autonomous_paused_ = false;
  uint32_t autonomous_pause_started_ms_ = 0;
  AutonomousLifecycle last_autonomous_lifecycle_ = AutonomousLifecycle::NONE;
  uint32_t autonomous_lifecycle_seq_ = 0;
  uint32_t autonomous_lifecycle_ms_ = 0;
  MicroBehaviorFrame micro_behavior_{};
};

}  // namespace ghost
}  // namespace deskbot

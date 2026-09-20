#pragma once

#include <Arduino.h>
#include "../device/CoreS3ImuDriver.h"
#include "../nerve/NerveTypes.h"

namespace deskbot {
namespace adapter {

class ImuAdapter {
 public:
  void begin(uint32_t now_ms);
  bool toNeuron(const device::ImuSample& sample,
                nerve::SemanticNeuron& out_neuron);

 private:
  enum class MotionState : uint8_t {
    SEEKING_REST = 0,
    REST_ARMED,
    LIFT_CANDIDATE,
    HELD,
    SETDOWN_CANDIDATE,
  };

  static float magnitude(float x, float y, float z);

  void resetRestDetection(uint32_t now_ms);
  void resetShakePattern();
  bool registerShakeImpulse(float dx, float dy, float dz, uint32_t now_ms);

  bool has_previous_ = false;
  float previous_ax_ = 0.0f;
  float previous_ay_ = 0.0f;
  float previous_az_ = 0.0f;

  MotionState motion_state_ = MotionState::SEEKING_REST;
  uint32_t rest_started_ms_ = 0;
  uint32_t lift_started_ms_ = 0;
  uint32_t lift_quiet_started_ms_ = 0;
  uint32_t setdown_quiet_started_ms_ = 0;
  bool setdown_impact_seen_ = false;

  uint32_t pickup_suppress_until_ms_ = 0;
  uint32_t last_shake_ms_ = 0;

  bool have_shake_impulse_ = false;
  float last_shake_dx_ = 0.0f;
  float last_shake_dy_ = 0.0f;
  float last_shake_dz_ = 0.0f;
  uint32_t last_shake_impulse_ms_ = 0;
  uint8_t shake_reversal_count_ = 0;
};

}  // namespace adapter
}  // namespace deskbot

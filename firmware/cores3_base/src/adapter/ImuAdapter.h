#pragma once

#include <Arduino.h>
#include "../device/CoreS3ImuDriver.h"
#include "../nerve/NerveTypes.h"

namespace deskbot {
namespace adapter {

struct NeckEfferenceCopy {
  uint32_t sequence = 0;
  uint32_t command_ms = 0;
  float previous_pitch_deg = 0.0f;
  float target_pitch_deg = 0.0f;

  float pitchDeltaDeg() const {
    return target_pitch_deg - previous_pitch_deg;
  }
};

class ImuAdapter {
 public:
  void begin(uint32_t now_ms);
  void setSelfMotionCommand(const NeckEfferenceCopy& command);
  bool toNeuron(const device::ImuSample& sample,
                nerve::SemanticNeuron& out_neuron);

  const char* debugStateName() const;
  uint8_t debugShakeReversalCount() const { return shake_reversal_count_; }
  float debugDeltaG() const { return debug_delta_g_; }
  float debugRawDeltaG() const { return debug_raw_delta_g_; }
  float debugSelfMotionCompensationG() const {
    return debug_self_motion_compensation_g_;
  }
  float debugMagnitudeDeviationG() const { return debug_magnitude_deviation_g_; }
  bool debugQuiet() const { return debug_quiet_; }

 private:
  enum class MotionState : uint8_t {
    SEEKING_REST = 0,
    REST_ARMED,
    LIFT_CANDIDATE,
    HELD,
    SETDOWN_CANDIDATE,
  };

  static float magnitude(float x, float y, float z);
  static void rotateAroundX(float x,
                            float y,
                            float z,
                            float angle_rad,
                            float& out_x,
                            float& out_y,
                            float& out_z);

  static const char* motionStateName(MotionState state);
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
  uint32_t lift_last_motion_ms_ = 0;
  uint8_t lift_motion_samples_ = 0;
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

  float debug_delta_g_ = 0.0f;
  float debug_raw_delta_g_ = 0.0f;
  float debug_self_motion_compensation_g_ = 0.0f;
  float debug_magnitude_deviation_g_ = 0.0f;
  bool debug_quiet_ = false;

  uint32_t last_self_motion_sequence_ = 0;
  uint32_t self_motion_command_ms_ = 0;
  float self_motion_pitch_delta_deg_ = 0.0f;
  float self_motion_last_progress_ = 1.0f;
  int8_t pitch_gravity_sign_ = 0;  // learned from first clear self-motion: -1/+1
};

}  // namespace adapter
}  // namespace deskbot

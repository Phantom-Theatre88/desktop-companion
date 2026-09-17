#include "ImuAdapter.h"

#include <math.h>

namespace deskbot {
namespace adapter {

namespace {

// LOCK 52: implementation tuning values for the standalone DeskRobo IMU path.
// These remain real-hardware tuning parameters, not personality LOCK values.
constexpr float kRestDeltaG = 0.05f;
constexpr float kRestMagnitudeDeviationG = 0.10f;
constexpr uint32_t kRestArmMs = 500;

constexpr float kLiftDeltaG = 0.16f;
constexpr float kLiftMagnitudeDeviationG = 0.14f;
constexpr uint32_t kLiftConfirmQuietMs = 180;
constexpr uint32_t kLiftCandidateTimeoutMs = 1200;

constexpr float kShakeDeltaG = 0.70f;
constexpr float kShakeMagnitudeDeviationG = 0.55f;
constexpr uint32_t kShakeCooldownMs = 900;
constexpr uint32_t kPostShakePickupSuppressMs = 1200;

// Once PICKED_UP has been emitted, IMU-only sensing cannot distinguish a
// quietly-held device from one resting on the desk. Do not re-arm pickup just
// because the device becomes quiet. Require a placement-like impact followed
// by sustained quiet before returning to REST_ARMED.
constexpr float kSetdownImpactDeltaG = 0.22f;
constexpr float kSetdownImpactMagnitudeDeviationG = 0.18f;
constexpr uint32_t kSetdownQuietConfirmMs = 700;
constexpr uint32_t kSetdownCandidateTimeoutMs = 1800;

}  // namespace

void ImuAdapter::begin(uint32_t now_ms) {
  has_previous_ = false;
  previous_ax_ = 0.0f;
  previous_ay_ = 0.0f;
  previous_az_ = 0.0f;

  motion_state_ = MotionState::SEEKING_REST;
  rest_started_ms_ = now_ms;
  lift_started_ms_ = 0;
  lift_quiet_started_ms_ = 0;
  setdown_quiet_started_ms_ = 0;
  setdown_impact_seen_ = false;

  pickup_suppress_until_ms_ = 0;
  last_shake_ms_ = now_ms - kShakeCooldownMs;
}

bool ImuAdapter::toNeuron(const device::ImuSample& sample,
                          nerve::SemanticNeuron& out_neuron) {
  if (!sample.available || !sample.valid) {
    return false;
  }

  if (!has_previous_) {
    previous_ax_ = sample.ax;
    previous_ay_ = sample.ay;
    previous_az_ = sample.az;
    has_previous_ = true;
    rest_started_ms_ = sample.timestamp_ms;
    return false;
  }

  const float dx = sample.ax - previous_ax_;
  const float dy = sample.ay - previous_ay_;
  const float dz = sample.az - previous_az_;
  const float delta_g = magnitude(dx, dy, dz);
  const float accel_g = magnitude(sample.ax, sample.ay, sample.az);
  const float magnitude_deviation_g = fabsf(accel_g - 1.0f);

  previous_ax_ = sample.ax;
  previous_ay_ = sample.ay;
  previous_az_ = sample.az;

  const float motion_strength =
      delta_g > magnitude_deviation_g ? delta_g : magnitude_deviation_g;

  const bool quiet =
      delta_g <= kRestDeltaG &&
      magnitude_deviation_g <= kRestMagnitudeDeviationG;

  const bool lift_motion =
      delta_g >= kLiftDeltaG ||
      magnitude_deviation_g >= kLiftMagnitudeDeviationG;

  const bool setdown_impact =
      delta_g >= kSetdownImpactDeltaG ||
      magnitude_deviation_g >= kSetdownImpactMagnitudeDeviationG;

  const bool shake_candidate =
      delta_g >= kShakeDeltaG ||
      magnitude_deviation_g >= kShakeMagnitudeDeviationG;

  // Strong abrupt motion has semantic priority. Crucially, SHAKE no longer
  // resets the pose state back to SEEKING_REST. If the device was already held,
  // it stays held; this prevents a later quiet period from becoming a false
  // second PICKED_UP for the same physical handling episode.
  if (shake_candidate &&
      (sample.timestamp_ms - last_shake_ms_) >= kShakeCooldownMs) {
    last_shake_ms_ = sample.timestamp_ms;
    pickup_suppress_until_ms_ = sample.timestamp_ms + kPostShakePickupSuppressMs;

    if (motion_state_ == MotionState::REST_ARMED ||
        motion_state_ == MotionState::LIFT_CANDIDATE ||
        motion_state_ == MotionState::SEEKING_REST) {
      motion_state_ = MotionState::HELD;
      setdown_impact_seen_ = false;
      setdown_quiet_started_ms_ = 0;
    }

    nerve::NeuronPayload payload;
    payload.scalar = motion_strength;

    out_neuron = nerve::makeNeuron(
        nerve::NeuronType::SHAKE,
        nerve::NeuronSource::IMU,
        sample.timestamp_ms,
        1.0f,
        payload);
    return true;
  }

  switch (motion_state_) {
    case MotionState::SEEKING_REST:
      if (quiet) {
        if (rest_started_ms_ == 0) {
          rest_started_ms_ = sample.timestamp_ms;
        }
        if ((sample.timestamp_ms - rest_started_ms_) >= kRestArmMs) {
          motion_state_ = MotionState::REST_ARMED;
        }
      } else {
        rest_started_ms_ = 0;
      }
      break;

    case MotionState::REST_ARMED:
      if (sample.timestamp_ms < pickup_suppress_until_ms_) {
        break;
      }

      if (lift_motion) {
        motion_state_ = MotionState::LIFT_CANDIDATE;
        lift_started_ms_ = sample.timestamp_ms;
        lift_quiet_started_ms_ = 0;
      }
      break;

    case MotionState::LIFT_CANDIDATE:
      if ((sample.timestamp_ms - lift_started_ms_) > kLiftCandidateTimeoutMs) {
        resetRestDetection(sample.timestamp_ms);
        break;
      }

      if (lift_motion) {
        lift_quiet_started_ms_ = 0;
        break;
      }

      if (quiet) {
        if (lift_quiet_started_ms_ == 0) {
          lift_quiet_started_ms_ = sample.timestamp_ms;
        }

        if ((sample.timestamp_ms - lift_quiet_started_ms_) >=
            kLiftConfirmQuietMs) {
          motion_state_ = MotionState::HELD;
          setdown_impact_seen_ = false;
          setdown_quiet_started_ms_ = 0;
          lift_started_ms_ = 0;
          lift_quiet_started_ms_ = 0;

          nerve::NeuronPayload payload;
          payload.scalar = motion_strength;

          out_neuron = nerve::makeNeuron(
              nerve::NeuronType::PICKED_UP,
              nerve::NeuronSource::IMU,
              sample.timestamp_ms,
              0.80f,
              payload);
          return true;
        }
      } else {
        lift_quiet_started_ms_ = 0;
      }
      break;

    case MotionState::HELD:
      // Quiet while being held is not evidence that the robot is back on the
      // desk. A set-down must first show a placement-like impact.
      if (setdown_impact) {
        motion_state_ = MotionState::SETDOWN_CANDIDATE;
        setdown_impact_seen_ = true;
        setdown_quiet_started_ms_ = 0;
        lift_started_ms_ = sample.timestamp_ms;  // reuse as candidate start
      }
      break;

    case MotionState::SETDOWN_CANDIDATE:
      if ((sample.timestamp_ms - lift_started_ms_) > kSetdownCandidateTimeoutMs) {
        // Ambiguous movement: assume still held rather than falsely re-arming.
        motion_state_ = MotionState::HELD;
        setdown_impact_seen_ = false;
        setdown_quiet_started_ms_ = 0;
        break;
      }

      if (!setdown_impact_seen_) {
        motion_state_ = MotionState::HELD;
        break;
      }

      if (quiet) {
        if (setdown_quiet_started_ms_ == 0) {
          setdown_quiet_started_ms_ = sample.timestamp_ms;
        }

        if ((sample.timestamp_ms - setdown_quiet_started_ms_) >=
            kSetdownQuietConfirmMs) {
          resetRestDetection(sample.timestamp_ms);
        }
      } else if (setdown_impact) {
        // Multiple small contacts while placing down: restart quiet confirm.
        setdown_quiet_started_ms_ = 0;
      }
      break;
  }

  return false;
}

void ImuAdapter::resetRestDetection(uint32_t now_ms) {
  motion_state_ = MotionState::SEEKING_REST;
  rest_started_ms_ = now_ms;
  lift_started_ms_ = 0;
  lift_quiet_started_ms_ = 0;
  setdown_quiet_started_ms_ = 0;
  setdown_impact_seen_ = false;
}

float ImuAdapter::magnitude(float x, float y, float z) {
  return sqrtf((x * x) + (y * y) + (z * z));
}

}  // namespace adapter
}  // namespace deskbot

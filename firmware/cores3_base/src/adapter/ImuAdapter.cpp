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
constexpr uint32_t kShakeImpulseWindowMs = 450;
constexpr uint8_t kShakeRequiredReversals = 1;

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
  resetShakePattern();
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

  debug_delta_g_ = delta_g;
  debug_magnitude_deviation_g_ = magnitude_deviation_g;
  debug_quiet_ = quiet;

  const bool lift_motion =
      delta_g >= kLiftDeltaG ||
      magnitude_deviation_g >= kLiftMagnitudeDeviationG;

  const bool setdown_impact =
      delta_g >= kSetdownImpactDeltaG ||
      magnitude_deviation_g >= kSetdownImpactMagnitudeDeviationG;

  const bool shake_candidate =
      delta_g >= kShakeDeltaG ||
      magnitude_deviation_g >= kShakeMagnitudeDeviationG;

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
        // First movement from rest immediately becomes a body/reflex cue.
        // This is NOT the confirmed PICKED_UP state: Ghost/Heart only receives
        // PICKED_UP after the existing lift-confirmation path succeeds.
        motion_state_ = MotionState::LIFT_CANDIDATE;
        lift_started_ms_ = sample.timestamp_ms;
        lift_quiet_started_ms_ = 0;
        resetShakePattern();
        if (shake_candidate) {
          registerShakeImpulse(dx, dy, dz, sample.timestamp_ms);
        }

        nerve::NeuronPayload payload;
        payload.scalar = motion_strength;
        out_neuron = nerve::makeNeuron(
            nerve::NeuronType::LIFT_STARTED,
            nerve::NeuronSource::IMU,
            sample.timestamp_ms,
            0.80f,
            payload);
        return true;
      }
      break;

    case MotionState::LIFT_CANDIDATE:
      if ((sample.timestamp_ms - lift_started_ms_) > kLiftCandidateTimeoutMs) {
        resetRestDetection(sample.timestamp_ms);
        break;
      }

      if (shake_candidate &&
          registerShakeImpulse(dx, dy, dz, sample.timestamp_ms) &&
          (sample.timestamp_ms - last_shake_ms_) >= kShakeCooldownMs) {
        last_shake_ms_ = sample.timestamp_ms;
        pickup_suppress_until_ms_ =
            sample.timestamp_ms + kPostShakePickupSuppressMs;
        motion_state_ = MotionState::HELD;
        setdown_impact_seen_ = false;
        setdown_quiet_started_ms_ = 0;

        nerve::NeuronPayload payload;
        payload.scalar = motion_strength;
        out_neuron = nerve::makeNeuron(
            nerve::NeuronType::SHAKE,
            nerve::NeuronSource::IMU,
            sample.timestamp_ms,
            1.0f,
            payload);
        resetShakePattern();
        return true;
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
      // While already held, SHAKE still requires repeated directional
      // reversals. A single jolt or a set-down impact is not enough.
      if (shake_candidate &&
          registerShakeImpulse(dx, dy, dz, sample.timestamp_ms) &&
          (sample.timestamp_ms - last_shake_ms_) >= kShakeCooldownMs) {
        last_shake_ms_ = sample.timestamp_ms;
        nerve::NeuronPayload payload;
        payload.scalar = motion_strength;
        out_neuron = nerve::makeNeuron(
            nerve::NeuronType::SHAKE,
            nerve::NeuronSource::IMU,
            sample.timestamp_ms,
            1.0f,
            payload);
        resetShakePattern();
        return true;
      }

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
  resetShakePattern();
}

void ImuAdapter::resetShakePattern() {
  have_shake_impulse_ = false;
  last_shake_dx_ = 0.0f;
  last_shake_dy_ = 0.0f;
  last_shake_dz_ = 0.0f;
  last_shake_impulse_ms_ = 0;
  shake_reversal_count_ = 0;
}

bool ImuAdapter::registerShakeImpulse(float dx, float dy, float dz,
                                      uint32_t now_ms) {
  if (!have_shake_impulse_ ||
      (now_ms - last_shake_impulse_ms_) > kShakeImpulseWindowMs) {
    have_shake_impulse_ = true;
    last_shake_dx_ = dx;
    last_shake_dy_ = dy;
    last_shake_dz_ = dz;
    last_shake_impulse_ms_ = now_ms;
    shake_reversal_count_ = 0;
    return false;
  }

  const float dot =
      dx * last_shake_dx_ + dy * last_shake_dy_ + dz * last_shake_dz_;
  if (dot < 0.0f) {
    ++shake_reversal_count_;
  }

  last_shake_dx_ = dx;
  last_shake_dy_ = dy;
  last_shake_dz_ = dz;
  last_shake_impulse_ms_ = now_ms;

  return shake_reversal_count_ >= kShakeRequiredReversals;
}

const char* ImuAdapter::motionStateName(MotionState state) {
  switch (state) {
    case MotionState::SEEKING_REST: return "SEEKING_REST";
    case MotionState::REST_ARMED: return "REST_ARMED";
    case MotionState::LIFT_CANDIDATE: return "LIFT_CANDIDATE";
    case MotionState::HELD: return "HELD";
    case MotionState::SETDOWN_CANDIDATE: return "SETDOWN_CANDIDATE";
    default: return "UNKNOWN";
  }
}

const char* ImuAdapter::debugStateName() const {
  return motionStateName(motion_state_);
}

float ImuAdapter::magnitude(float x, float y, float z) {
  return sqrtf((x * x) + (y * y) + (z * z));
}

}  // namespace adapter
}  // namespace deskbot

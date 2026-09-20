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

// Efference-copy compensation:
// Predict how the gravity vector should rotate in IMU coordinates from our own
// commanded neck pitch, then subtract that VECTOR from the measured change.
// This is intentionally not a scalar "ignore window" or delta-g budget.
// The SCS command uses a short move time; model it over a slightly wider window
// so two or three IMU samples can share the predicted rotation.
constexpr uint32_t kSelfMotionModelMs = 60;
constexpr float kSelfMotionMinLearnDeg = 0.35f;
constexpr float kSelfMotionLearnMarginG = 0.0025f;

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

  last_self_motion_sequence_ = 0;
  self_motion_command_ms_ = 0;
  self_motion_pitch_delta_deg_ = 0.0f;
  self_motion_last_progress_ = 1.0f;
  pitch_gravity_sign_ = 0;
}

void ImuAdapter::setSelfMotionCommand(
    const device::NeckMotionCommand& command) {
  if (command.sequence == 0 ||
      command.sequence == last_self_motion_sequence_) {
    return;
  }

  last_self_motion_sequence_ = command.sequence;
  self_motion_command_ms_ = command.command_ms;
  self_motion_pitch_delta_deg_ = command.pitchDeltaDeg();
  self_motion_last_progress_ = 0.0f;
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

  const float raw_dx = sample.ax - previous_ax_;
  const float raw_dy = sample.ay - previous_ay_;
  const float raw_dz = sample.az - previous_az_;
  const float raw_delta_g = magnitude(raw_dx, raw_dy, raw_dz);
  const float accel_g = magnitude(sample.ax, sample.ay, sample.az);
  const float magnitude_deviation_g = fabsf(accel_g - 1.0f);

  float predicted_x = previous_ax_;
  float predicted_y = previous_ay_;
  float predicted_z = previous_az_;
  float compensation_g = 0.0f;

  // Convert this sample's fraction of our commanded pitch into an expected
  // gravity-vector rotation. We learn the physical servo/IMU sign once from
  // the first clear movement by comparing +pitch vs -pitch predictions.
  if (self_motion_last_progress_ < 1.0f &&
      self_motion_command_ms_ != 0) {
    const uint32_t elapsed_ms = sample.timestamp_ms - self_motion_command_ms_;
    float progress =
        static_cast<float>(elapsed_ms) /
        static_cast<float>(kSelfMotionModelMs);
    if (progress < 0.0f) progress = 0.0f;
    if (progress > 1.0f) progress = 1.0f;

    const float progress_delta = progress - self_motion_last_progress_;
    if (progress_delta > 0.0f) {
      const float incremental_pitch_deg =
          self_motion_pitch_delta_deg_ * progress_delta;
      const float angle_rad =
          incremental_pitch_deg * 3.14159265f / 180.0f;

      float plus_x = previous_ax_;
      float plus_y = previous_ay_;
      float plus_z = previous_az_;
      float minus_x = previous_ax_;
      float minus_y = previous_ay_;
      float minus_z = previous_az_;

      rotateAroundX(previous_ax_, previous_ay_, previous_az_,
                    angle_rad, plus_x, plus_y, plus_z);
      rotateAroundX(previous_ax_, previous_ay_, previous_az_,
                    -angle_rad, minus_x, minus_y, minus_z);

      if (pitch_gravity_sign_ == 0 &&
          fabsf(incremental_pitch_deg) >= kSelfMotionMinLearnDeg) {
        const float plus_error =
            magnitude(sample.ax - plus_x,
                      sample.ay - plus_y,
                      sample.az - plus_z);
        const float minus_error =
            magnitude(sample.ax - minus_x,
                      sample.ay - minus_y,
                      sample.az - minus_z);
        if (fabsf(plus_error - minus_error) >= kSelfMotionLearnMarginG) {
          pitch_gravity_sign_ = plus_error < minus_error ? 1 : -1;
        }
      }

      const int8_t sign = pitch_gravity_sign_ == 0 ? 1 : pitch_gravity_sign_;
      if (sign > 0) {
        predicted_x = plus_x;
        predicted_y = plus_y;
        predicted_z = plus_z;
      } else {
        predicted_x = minus_x;
        predicted_y = minus_y;
        predicted_z = minus_z;
      }

      compensation_g =
          magnitude(predicted_x - previous_ax_,
                    predicted_y - previous_ay_,
                    predicted_z - previous_az_);
    }

    self_motion_last_progress_ = progress;
  }

  // External-motion residual = measured acceleration vector minus the vector
  // expected from our own neck rotation.
  const float dx = sample.ax - predicted_x;
  const float dy = sample.ay - predicted_y;
  const float dz = sample.az - predicted_z;
  const float delta_g = magnitude(dx, dy, dz);

  previous_ax_ = sample.ax;
  previous_ay_ = sample.ay;
  previous_az_ = sample.az;

  const float motion_strength =
      delta_g > magnitude_deviation_g ? delta_g : magnitude_deviation_g;

  const bool quiet =
      delta_g <= kRestDeltaG &&
      magnitude_deviation_g <= kRestMagnitudeDeviationG;

  debug_raw_delta_g_ = raw_delta_g;
  debug_self_motion_compensation_g_ = compensation_g;
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

void ImuAdapter::rotateAroundX(float x,
                               float y,
                               float z,
                               float angle_rad,
                               float& out_x,
                               float& out_y,
                               float& out_z) {
  const float c = cosf(angle_rad);
  const float s = sinf(angle_rad);
  out_x = x;
  out_y = (y * c) - (z * s);
  out_z = (y * s) + (z * c);
}

float ImuAdapter::magnitude(float x, float y, float z) {
  return sqrtf((x * x) + (y * y) + (z * z));
}

}  // namespace adapter
}  // namespace deskbot

#include "ImuAdapter.h"

#include <math.h>

namespace deskbot {
namespace adapter {

namespace {

// LOCK 52: implementation tuning values for the standalone DeskRobo IMU path.
// These are deliberately adjustable from real CoreS3 observation and are not
// personality LOCK values.
constexpr float kRestDeltaG = 0.05f;
constexpr float kRestMagnitudeDeviationG = 0.10f;
constexpr uint32_t kRestArmMs = 500;

constexpr float kLiftDeltaG = 0.16f;
constexpr float kLiftMagnitudeDeviationG = 0.14f;
constexpr uint32_t kLiftConfirmQuietMs = 180;
constexpr uint32_t kLiftCandidateTimeoutMs = 1200;
constexpr uint32_t kPostPickupSuppressMs = 1800;

constexpr float kShakeDeltaG = 0.70f;
constexpr float kShakeMagnitudeDeviationG = 0.55f;
constexpr uint32_t kShakeCooldownMs = 900;
constexpr uint32_t kPostShakeSuppressMs = 1200;

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
  suppress_until_ms_ = 0;
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

  const bool shake_candidate =
      delta_g >= kShakeDeltaG ||
      magnitude_deviation_g >= kShakeMagnitudeDeviationG;

  // Strong abrupt motion has priority. Once SHAKE is recognized, pickup
  // inference is suppressed for a short period so the same physical action
  // cannot immediately become PICKED_UP as it settles.
  if (shake_candidate &&
      (sample.timestamp_ms - last_shake_ms_) >= kShakeCooldownMs) {
    last_shake_ms_ = sample.timestamp_ms;
    motion_state_ = MotionState::POST_EVENT_SUPPRESS;
    suppress_until_ms_ = sample.timestamp_ms + kPostShakeSuppressMs;
    rest_started_ms_ = 0;
    lift_started_ms_ = 0;
    lift_quiet_started_ms_ = 0;

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

  if (motion_state_ == MotionState::POST_EVENT_SUPPRESS) {
    if (sample.timestamp_ms < suppress_until_ms_) {
      return false;
    }
    resetRestDetection(sample.timestamp_ms);
  }

  const bool quiet =
      delta_g <= kRestDeltaG &&
      magnitude_deviation_g <= kRestMagnitudeDeviationG;

  const bool lift_motion =
      delta_g >= kLiftDeltaG ||
      magnitude_deviation_g >= kLiftMagnitudeDeviationG;

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
      // PICKED_UP is not inferred from motion magnitude alone anymore. The
      // device must first have been still, then start a moderate movement.
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

      // Confirm only when the initial movement settles into a quiet hold.
      // This matches the semantic sequence: resting -> lifted -> held.
      if (quiet) {
        if (lift_quiet_started_ms_ == 0) {
          lift_quiet_started_ms_ = sample.timestamp_ms;
        }

        if ((sample.timestamp_ms - lift_quiet_started_ms_) >=
            kLiftConfirmQuietMs) {
          motion_state_ = MotionState::POST_EVENT_SUPPRESS;
          suppress_until_ms_ = sample.timestamp_ms + kPostPickupSuppressMs;
          rest_started_ms_ = 0;
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

    case MotionState::POST_EVENT_SUPPRESS:
      // handled above
      break;
  }

  return false;
}

void ImuAdapter::resetRestDetection(uint32_t now_ms) {
  motion_state_ = MotionState::SEEKING_REST;
  rest_started_ms_ = now_ms;
  lift_started_ms_ = 0;
  lift_quiet_started_ms_ = 0;
  suppress_until_ms_ = 0;
}

float ImuAdapter::magnitude(float x, float y, float z) {
  return sqrtf((x * x) + (y * y) + (z * z));
}

}  // namespace adapter
}  // namespace deskbot

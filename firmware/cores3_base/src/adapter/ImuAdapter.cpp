#include "ImuAdapter.h"

#include <math.h>

namespace deskbot {
namespace adapter {

namespace {

// LOCK 52: first standalone DeskRobo implementation tuning values.
// These are not personality LOCKs and are expected to be adjusted by real
// CoreS3 observation.
constexpr float kPickupDeltaG = 0.18f;
constexpr float kPickupMagnitudeDeviationG = 0.16f;
constexpr uint8_t kPickupRequiredHits = 2;
constexpr uint32_t kPickupCooldownMs = 1200;

constexpr float kShakeDeltaG = 0.70f;
constexpr float kShakeMagnitudeDeviationG = 0.55f;
constexpr uint32_t kShakeCooldownMs = 900;

}  // namespace

void ImuAdapter::begin(uint32_t now_ms) {
  has_previous_ = false;
  previous_ax_ = 0.0f;
  previous_ay_ = 0.0f;
  previous_az_ = 0.0f;
  pickup_motion_hits_ = 0;
  last_pickup_ms_ = now_ms - kPickupCooldownMs;
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

  // Strong, abrupt motion takes priority over PICKED_UP. The adapter converts
  // device-specific acceleration into the product-independent SHAKE meaning.
  const bool shake_candidate =
      delta_g >= kShakeDeltaG ||
      magnitude_deviation_g >= kShakeMagnitudeDeviationG;
  if (shake_candidate &&
      (sample.timestamp_ms - last_shake_ms_) >= kShakeCooldownMs) {
    last_shake_ms_ = sample.timestamp_ms;
    pickup_motion_hits_ = 0;

    nerve::NeuronPayload payload;
    payload.scalar = delta_g > magnitude_deviation_g ? delta_g : magnitude_deviation_g;

    out_neuron = nerve::makeNeuron(
        nerve::NeuronType::SHAKE,
        nerve::NeuronSource::IMU,
        sample.timestamp_ms,
        1.0f,
        payload);
    return true;
  }

  const bool pickup_motion =
      delta_g >= kPickupDeltaG ||
      magnitude_deviation_g >= kPickupMagnitudeDeviationG;

  if (pickup_motion) {
    if (pickup_motion_hits_ < 255) {
      ++pickup_motion_hits_;
    }
  } else {
    pickup_motion_hits_ = 0;
  }

  // IMU alone cannot literally know whether a hand is holding the device.
  // This is therefore a conservative motion-derived PICKED_UP semantic hint:
  // require repeated moderate movement and apply a cooldown to prevent floods.
  if (pickup_motion_hits_ >= kPickupRequiredHits &&
      (sample.timestamp_ms - last_pickup_ms_) >= kPickupCooldownMs) {
    last_pickup_ms_ = sample.timestamp_ms;
    pickup_motion_hits_ = 0;

    nerve::NeuronPayload payload;
    payload.scalar = delta_g > magnitude_deviation_g ? delta_g : magnitude_deviation_g;

    out_neuron = nerve::makeNeuron(
        nerve::NeuronType::PICKED_UP,
        nerve::NeuronSource::IMU,
        sample.timestamp_ms,
        0.75f,
        payload);
    return true;
  }

  return false;
}

float ImuAdapter::magnitude(float x, float y, float z) {
  return sqrtf((x * x) + (y * y) + (z * z));
}

}  // namespace adapter
}  // namespace deskbot

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
  static float magnitude(float x, float y, float z);

  bool has_previous_ = false;
  float previous_ax_ = 0.0f;
  float previous_ay_ = 0.0f;
  float previous_az_ = 0.0f;
  uint8_t pickup_motion_hits_ = 0;
  uint32_t last_pickup_ms_ = 0;
  uint32_t last_shake_ms_ = 0;
};

}  // namespace adapter
}  // namespace deskbot

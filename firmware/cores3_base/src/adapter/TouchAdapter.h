#pragma once

#include <Arduino.h>
#include "../device/CoreS3TouchDriver.h"
#include "../nerve/NerveTypes.h"

namespace deskbot {
namespace adapter {

class TouchAdapter {
 public:
  bool toNeuron(const device::TouchSample& sample,
                nerve::SemanticNeuron& out_neuron) const;
};

}  // namespace adapter
}  // namespace deskbot

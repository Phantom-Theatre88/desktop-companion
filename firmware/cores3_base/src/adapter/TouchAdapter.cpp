#include "TouchAdapter.h"

namespace deskbot {
namespace adapter {

bool TouchAdapter::toNeuron(const device::TouchSample& sample,
                            nerve::SemanticNeuron& out_neuron) const {
  if (!sample.available || !sample.pressed) {
    return false;
  }

  nerve::NeuronPayload payload;
  payload.x = sample.x;
  payload.y = sample.y;

  out_neuron = nerve::makeNeuron(
      nerve::NeuronType::TOUCH,
      nerve::NeuronSource::TOUCH,
      sample.timestamp_ms,
      1.0f,
      payload);
  return true;
}

}  // namespace adapter
}  // namespace deskbot

#include "WakeWordAdapter.h"

namespace deskbot {
namespace adapter {

void WakeWordAdapter::begin(const char* target_word) {
  if (target_word != nullptr && target_word[0] != '\0') {
    target_word_ = target_word;
  }
}

void WakeWordAdapter::setDetector(
    WakeWordDetectHandler handler,
    void* context) {
  detector_ = handler;
  detector_context_ = context;
}

bool WakeWordAdapter::toNeuron(
    const device::MicFrameView& frame,
    nerve::SemanticNeuron& out_neuron) {
  if (detector_ == nullptr ||
      !frame.available || !frame.valid ||
      frame.pcm == nullptr || frame.sample_count == 0) {
    return false;
  }

  float confidence = 0.0f;
  if (!detector_(frame.pcm,
                 frame.sample_count,
                 frame.sample_rate,
                 confidence,
                 detector_context_)) {
    return false;
  }

  if (confidence < 0.0f) confidence = 0.0f;
  if (confidence > 1.0f) confidence = 1.0f;

  nerve::NeuronPayload payload;
  payload.scalar = confidence;
  out_neuron = nerve::makeNeuron(
      nerve::NeuronType::WAKE_WORD_DETECTED,
      nerve::NeuronSource::MIC,
      frame.timestamp_ms,
      confidence,
      payload);
  return true;
}

}  // namespace adapter
}  // namespace deskbot

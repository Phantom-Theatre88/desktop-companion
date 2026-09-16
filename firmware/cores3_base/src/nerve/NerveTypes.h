#pragma once

#include <Arduino.h>

namespace deskbot {
namespace nerve {

enum class NeuronType : uint16_t {
  NONE = 0,

  // Presence / proximity
  PROXIMITY_NEAR,
  PROXIMITY_APPROACHING,
  PROXIMITY_LEAVING,
  PERSON_PRESENT,
  PERSON_ABSENT,

  // Touch / body
  TOUCH,
  STROKE_DETECTED,
  PICKED_UP,
  SHAKE,

  // Vision / audio
  FACE_DETECTED,
  FACE_LOST,
  LOUD_SOUND,
  VOICE_ACTIVITY,

  // Higher-level requests / states
  LOOK_AT,
  ATTENTION_REQUEST,
};

enum class NeuronSource : uint8_t {
  UNKNOWN = 0,
  TOF,
  TMOS,
  ENV,
  TOUCH,
  IMU,
  MIC,
  CAMERA_M5,
  CAMERA_PI5,
  PI5,
  GHOST,
};

struct NeuronPayload {
  // Optional diagnostic/context payload. Semantic meaning must remain in type.
  // Raw device values must not be required by downstream consumers.
  float scalar = 0.0f;
  int32_t x = 0;
  int32_t y = 0;
};

struct SemanticNeuron {
  NeuronType type = NeuronType::NONE;
  NeuronSource source = NeuronSource::UNKNOWN;
  uint32_t timestamp_ms = 0;
  float confidence = 1.0f;
  NeuronPayload payload{};
};

inline SemanticNeuron makeNeuron(
    NeuronType type,
    NeuronSource source,
    uint32_t timestamp_ms,
    float confidence = 1.0f,
    const NeuronPayload& payload = {}) {
  SemanticNeuron neuron;
  neuron.type = type;
  neuron.source = source;
  neuron.timestamp_ms = timestamp_ms;
  neuron.confidence = confidence;
  neuron.payload = payload;
  return neuron;
}

}  // namespace nerve
}  // namespace deskbot

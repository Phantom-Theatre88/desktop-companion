#pragma once

#include <cstdint>

#include "stackchan/neural/neural_circuit.h"

namespace yuki::senses::tof4m {

struct ToF4MConfig {
    float near_threshold_mm = 600.0f;
    float leave_threshold_mm = 750.0f;
    float approaching_delta_mm = 80.0f;
};

struct ToF4MSample {
    float distance_mm = 0.0f;
    uint64_t timestamp_ms = 0;
    float confidence = 1.0f;
    bool valid = false;
};

class ToF4MPerception {
public:
    explicit ToF4MPerception(neural::NeuralCircuit& circuit, ToF4MConfig config = {});

    void reset();
    void ingest(const ToF4MSample& sample);

private:
    enum class ProximityState : uint8_t {
        kUnknown = 0,
        kFar,
        kNear,
    };

    void emit(neural::SemanticEventId id, const ToF4MSample& sample, float delta_mm);

    neural::NeuralCircuit& circuit_;
    ToF4MConfig config_;
    ProximityState state_ = ProximityState::kUnknown;
    float previous_distance_mm_ = 0.0f;
    bool has_previous_ = false;
};

}  // namespace yuki::senses::tof4m

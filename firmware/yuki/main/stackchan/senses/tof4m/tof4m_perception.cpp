#include "stackchan/senses/tof4m/tof4m_perception.h"

namespace yuki::senses::tof4m {

ToF4MPerception::ToF4MPerception(neural::NeuralCircuit& circuit, ToF4MConfig config)
    : circuit_(circuit), config_(config) {}

void ToF4MPerception::reset() {
    state_ = ProximityState::kUnknown;
    previous_distance_mm_ = 0.0f;
    has_previous_ = false;
}

void ToF4MPerception::ingest(const ToF4MSample& sample) {
    if (!sample.valid) {
        return;
    }

    const float delta_mm = has_previous_ ? sample.distance_mm - previous_distance_mm_ : 0.0f;

    if (state_ == ProximityState::kUnknown) {
        state_ = sample.distance_mm <= config_.near_threshold_mm
                     ? ProximityState::kNear
                     : ProximityState::kFar;

        if (state_ == ProximityState::kNear) {
            emit(neural::SemanticEventId::kProximityNear, sample, delta_mm);
        }
    } else if (state_ == ProximityState::kFar) {
        if (has_previous_ && delta_mm <= -config_.approaching_delta_mm) {
            emit(neural::SemanticEventId::kProximityApproaching, sample, delta_mm);
        }

        if (sample.distance_mm <= config_.near_threshold_mm) {
            state_ = ProximityState::kNear;
            emit(neural::SemanticEventId::kProximityNear, sample, delta_mm);
        }
    } else if (state_ == ProximityState::kNear) {
        if (sample.distance_mm >= config_.leave_threshold_mm) {
            state_ = ProximityState::kFar;
            emit(neural::SemanticEventId::kProximityLeave, sample, delta_mm);
        }
    }

    previous_distance_mm_ = sample.distance_mm;
    has_previous_ = true;
}

void ToF4MPerception::emit(neural::SemanticEventId id, const ToF4MSample& sample, float delta_mm) {
    neural::SemanticEvent event;
    event.id = id;
    event.timestamp_ms = sample.timestamp_ms;
    event.confidence = sample.confidence;
    event.payload.distance_mm = sample.distance_mm;
    event.payload.delta_mm = delta_mm;
    circuit_.emit(event);
}

}  // namespace yuki::senses::tof4m

#include "stackchan/senses/tof4m/tof4m_perception.h"

#include <cmath>

namespace yuki::senses::tof4m {

ToF4MPerception::ToF4MPerception(neural::NeuralCircuit& circuit, ToF4MConfig config)
    : circuit_(circuit), config_(config) {}

void ToF4MPerception::reset() {
    state_ = ProximityState::kUnknown;
    previous_distance_mm_ = 0.0f;
    previous_timestamp_ms_ = 0;
    has_previous_ = false;
    approach_active_ = false;
}

void ToF4MPerception::ingest(const ToF4MSample& sample) {
    if (!sample.valid) {
        return;
    }

    float delta_mm = 0.0f;
    float speed_mm_s = 0.0f;
    bool speed_valid = false;

    if (has_previous_ && sample.timestamp_ms > previous_timestamp_ms_) {
        delta_mm = sample.distance_mm - previous_distance_mm_;
        const float dt_s = static_cast<float>(sample.timestamp_ms - previous_timestamp_ms_) / 1000.0f;
        if (dt_s > 0.0f) {
            speed_mm_s = delta_mm / dt_s;
            speed_valid = true;
        }
    }

    if (state_ == ProximityState::kUnknown) {
        state_ = sample.distance_mm <= config_.near_threshold_mm
                     ? ProximityState::kNear
                     : ProximityState::kFar;

        if (state_ == ProximityState::kNear) {
            emit(neural::SemanticEventId::kProximityNear, sample, delta_mm);
        }
    } else if (state_ == ProximityState::kFar) {
        const bool approaching_now = speed_valid && speed_mm_s <= -config_.approaching_speed_threshold_mm_s;

        if (approaching_now && !approach_active_) {
            approach_active_ = true;
            emit(neural::SemanticEventId::kProximityApproaching, sample, delta_mm);
        } else if (approach_active_ && speed_valid && speed_mm_s >= -config_.approach_release_speed_mm_s) {
            approach_active_ = false;
        }

        if (sample.distance_mm <= config_.near_threshold_mm) {
            state_ = ProximityState::kNear;
            approach_active_ = false;
            emit(neural::SemanticEventId::kProximityNear, sample, delta_mm);
        }
    } else if (state_ == ProximityState::kNear) {
        approach_active_ = false;
        if (sample.distance_mm >= config_.leave_threshold_mm) {
            state_ = ProximityState::kFar;
            emit(neural::SemanticEventId::kProximityLeave, sample, delta_mm);
        }
    }

    previous_distance_mm_ = sample.distance_mm;
    previous_timestamp_ms_ = sample.timestamp_ms;
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

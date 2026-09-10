#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace yuki::neural {

enum class SemanticEventId : uint16_t {
    kUnknown = 0,
    kProximityNear,
    kProximityApproaching,
    kProximityLeave,
};

struct SemanticEvent {
    SemanticEventId id = SemanticEventId::kUnknown;
    uint64_t timestamp_ms = 0;
    float confidence = 0.0f;

    struct Payload {
        float distance_mm = 0.0f;
        float delta_mm = 0.0f;
        float velocity_mm_s = 0.0f;
    } payload;
};

using SynapseCallback = void (*)(const SemanticEvent& event, void* context);

struct Synapse {
    SemanticEventId event_id = SemanticEventId::kUnknown;
    SynapseCallback callback = nullptr;
    void* context = nullptr;
    bool enabled = false;
};

class NeuralCircuit {
public:
    static constexpr std::size_t kMaxSynapses = 16;

    bool connect(SemanticEventId event_id, SynapseCallback callback, void* context = nullptr);
    void disconnect_all();
    void emit(const SemanticEvent& event) const;
    std::size_t connection_count() const;

private:
    std::array<Synapse, kMaxSynapses> synapses_{};
};

// Shared semantic nerve bus for Yuki. Sensors, reflexes, Heart and later Pi5
// adapters connect here instead of creating device-specific event paths.
NeuralCircuit& GetNeuralCircuit();
const char* SemanticEventName(SemanticEventId id);

}  // namespace yuki::neural

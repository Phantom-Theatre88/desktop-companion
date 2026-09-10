#include "stackchan/senses/tof4m/tof4m_runtime.h"

#include <atomic>

#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "stackchan/neural/neural_circuit.h"
#include "stackchan/senses/tof4m/tof4m_driver.h"
#include "stackchan/senses/tof4m/tof4m_perception.h"

namespace yuki::senses::tof4m {
namespace {

constexpr char kTag[] = "YukiToFNeural";
constexpr uint32_t kPollIntervalMs = 50;

std::atomic<bool> started{false};
neural::NeuralCircuit circuit;
ToF4MPerception perception(circuit);
ToF4MDriver driver;

const char* event_name(neural::SemanticEventId id) {
    switch (id) {
        case neural::SemanticEventId::kProximityApproaching:
            return "PROXIMITY_APPROACHING";
        case neural::SemanticEventId::kProximityNear:
            return "PROXIMITY_NEAR";
        case neural::SemanticEventId::kProximityLeave:
            return "PROXIMITY_LEAVE";
        default:
            return "UNKNOWN";
    }
}

void log_synapse(const neural::SemanticEvent& event, void*) {
    ESP_LOGI(kTag,
             "[SYNAPSE] %s distance=%.0fmm delta=%.0fmm confidence=%.2f",
             event_name(event.id),
             static_cast<double>(event.payload.distance_mm),
             static_cast<double>(event.payload.delta_mm),
             static_cast<double>(event.confidence));
}

void tof_task(void*) {
    const esp_err_t init_err = driver.init();
    if (init_err != ESP_OK) {
        ESP_LOGE(kTag, "U172 neural input init failed: %s", esp_err_to_name(init_err));
        started.store(false);
        vTaskDelete(nullptr);
        return;
    }

    circuit.connect(neural::SemanticEventId::kProximityApproaching, log_synapse);
    circuit.connect(neural::SemanticEventId::kProximityNear, log_synapse);
    circuit.connect(neural::SemanticEventId::kProximityLeave, log_synapse);

    ESP_LOGI(kTag, "Neuron/Synapse path armed (%u synapses)",
             static_cast<unsigned>(circuit.connection_count()));

    uint32_t last_distance_log_ms = 0;
    uint32_t failure_count = 0;

    while (true) {
        uint16_t distance_mm = 0;
        bool ready = false;
        const esp_err_t err = driver.read_distance_mm(distance_mm, ready);
        const uint64_t now_ms = static_cast<uint64_t>(esp_timer_get_time() / 1000);

        if (err != ESP_OK) {
            ++failure_count;
            if (failure_count == 1 || failure_count % 20 == 0) {
                ESP_LOGW(kTag, "U172 read failed x%u: %s",
                         static_cast<unsigned>(failure_count), esp_err_to_name(err));
            }
        } else {
            failure_count = 0;
            if (ready) {
                ToF4MSample sample;
                sample.distance_mm = static_cast<float>(distance_mm);
                sample.timestamp_ms = now_ms;
                sample.confidence = 1.0f;
                sample.valid = distance_mm >= 40 && distance_mm <= 4000;

                if (sample.valid) {
                    perception.ingest(sample);
                    if (last_distance_log_ms == 0 || now_ms - last_distance_log_ms >= 500) {
                        ESP_LOGI(kTag, "[TOF] distance=%umm", static_cast<unsigned>(distance_mm));
                        last_distance_log_ms = static_cast<uint32_t>(now_ms);
                    }
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(kPollIntervalMs));
    }
}

}  // namespace

void StartToF4MNeuralInput() {
    bool expected = false;
    if (!started.compare_exchange_strong(expected, true)) {
        return;
    }

    if (xTaskCreate(tof_task, "yuki_tof4m", 4096, nullptr, 3, nullptr) != pdPASS) {
        ESP_LOGE(kTag, "Unable to start U172 neural task");
        started.store(false);
    }
}

}  // namespace yuki::senses::tof4m

#pragma once

#include <Arduino.h>
#include "HeartPersistence.h"
#include "../nerve/NerveTypes.h"
#include "../reflex/ReflexLayer.h"

namespace deskbot {
namespace ghost {

struct HeartState {
  float mood = 0.60f;
  float affection = 0.55f;
  float curiosity = 0.65f;
  float boredom = 0.15f;
  float sleepiness = 0.15f;
  float attention = 0.50f;
};

struct HeartContext {
  HeartState state{};
  uint32_t captured_ms = 0;
};

class HeartEngine {
 public:
  void begin(uint32_t now_ms);
  void tick(uint32_t now_ms);
  void onNeuron(const nerve::SemanticNeuron& neuron, const HeartContext& event_context);
  void onReflexResult(const reflex::ReflexResult& result,
                      const HeartContext& result_context);

  HeartContext snapshot(uint32_t now_ms) const;
  const HeartState& state() const { return state_; }

  bool primarySnapshotLoaded() const { return primary_snapshot_loaded_; }
  bool firstBootInitialized() const { return first_boot_initialized_; }
  bool primarySaveOk() const { return primary_save_ok_; }
  const HeartState& primarySnapshot() const { return primary_snapshot_; }

 private:
  static float clamp01(float value);
  void clampState();

  HeartState state_{};
  HeartState primary_snapshot_{};
  HeartPersistence persistence_{};
  bool primary_snapshot_loaded_ = false;
  bool first_boot_initialized_ = false;
  bool primary_save_ok_ = false;
  uint32_t last_tick_ms_ = 0;
  nerve::NeuronType last_event_type_ = nerve::NeuronType::NONE;
  uint32_t last_event_ms_ = 0;
};

}  // namespace ghost
}  // namespace deskbot

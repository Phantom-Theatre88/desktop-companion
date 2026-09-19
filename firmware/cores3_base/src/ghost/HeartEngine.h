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

enum class HeartRestoreMode : uint8_t {
  PRESERVE_SAVED = 0,
  DECAY_TOWARD_DYNAMIC_BASELINE,
  RECALCULATE_FROM_ELAPSED_CONTEXT,
  RECALCULATE_FROM_TIME_RHYTHM,
  RESET_TO_DYNAMIC_BASELINE,
};

struct HeartRestorePlan {
  HeartRestoreMode mood = HeartRestoreMode::DECAY_TOWARD_DYNAMIC_BASELINE;
  HeartRestoreMode affection = HeartRestoreMode::PRESERVE_SAVED;
  HeartRestoreMode curiosity = HeartRestoreMode::DECAY_TOWARD_DYNAMIC_BASELINE;
  HeartRestoreMode boredom = HeartRestoreMode::RECALCULATE_FROM_ELAPSED_CONTEXT;
  HeartRestoreMode sleepiness = HeartRestoreMode::RECALCULATE_FROM_TIME_RHYTHM;
  HeartRestoreMode attention = HeartRestoreMode::RESET_TO_DYNAMIC_BASELINE;
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
  const HeartState& baseline() const { return baseline_; }
  nerve::NeuronType lastEventType() const { return last_event_type_; }
  uint32_t lastEventMs() const { return last_event_ms_; }
  uint32_t lastHeartChangeMs() const { return last_heart_change_ms_; }
  float lastImpactScale() const { return last_impact_scale_; }
  uint32_t lastRecoveryMs() const { return last_recovery_ms_; }

  bool primarySnapshotLoaded() const { return primary_snapshot_loaded_; }
  bool firstBootInitialized() const { return first_boot_initialized_; }
  bool primarySaveOk() const { return primary_save_ok_; }
  const HeartState& primarySnapshot() const { return primary_snapshot_; }

  // LOCK 20 production boundary. The plan fixes HOW each field must be restored
  // without inventing coefficients, elapsed-time rules, life-rhythm rules, or
  // long-term dynamic baselines before Step 9 has the required inputs.
  const HeartRestorePlan& restorePlan() const { return restore_plan_; }
  bool restorePending() const { return restore_pending_; }

  void setMicroSdBackupHandlers(HeartBackupSaveHandler save_handler,
                                HeartBackupLoadHandler load_handler,
                                void* context = nullptr);
  bool microSdBackupAvailable() const;
  bool saveMicroSdBackup() const;
  bool loadMicroSdBackup(HeartState& out_state) const;

 private:
  static float clamp01(float value);
  static float clampStrength(float value);
  static uint8_t stimulusFamily(nerve::NeuronType type);
  float habituationScaleFor(nerve::NeuronType type, uint32_t now_ms);
  bool applyRecoveryStep(uint32_t now_ms);
  void applyDelta(float mood,
                  float affection,
                  float curiosity,
                  float boredom,
                  float sleepiness,
                  float attention,
                  uint32_t now_ms);
  void clampState();

  HeartState state_{};
  HeartState baseline_{};
  HeartState primary_snapshot_{};
  HeartRestorePlan restore_plan_{};
  HeartPersistence persistence_{};
  bool primary_snapshot_loaded_ = false;
  bool first_boot_initialized_ = false;
  bool primary_save_ok_ = false;
  bool restore_pending_ = false;
  uint32_t last_tick_ms_ = 0;
  uint32_t last_heart_change_ms_ = 0;
  uint32_t last_boredom_step_ms_ = 0;
  uint32_t last_stimulus_ms_ = 0;
  uint8_t last_stimulus_family_ = 0;
  uint8_t repeated_stimulus_count_ = 0;
  float last_impact_scale_ = 1.0f;
  nerve::NeuronType last_event_type_ = nerve::NeuronType::NONE;
  uint32_t last_event_ms_ = 0;
};

}  // namespace ghost
}  // namespace deskbot

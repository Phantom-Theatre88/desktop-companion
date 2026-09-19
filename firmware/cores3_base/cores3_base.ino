#include <M5Unified.h>
#include <M5GFX.h>

#include "src/adapter/ImuAdapter.h"
#include "src/adapter/TouchAdapter.h"
#include "src/core/DesktopCompanionRuntime.h"
#include "src/device/CoreS3CameraDriver.h"
#include "src/device/CoreS3ImuDriver.h"
#include "src/device/CoreS3TouchDriver.h"
#include "src/face/FaceRenderer.h"
#include "src/vision/CameraVisionInput.h"

deskbot::core::DesktopCompanionRuntime runtime;
deskbot::face::FaceRenderer face_renderer;
deskbot::device::CoreS3TouchDriver touch_driver;
deskbot::adapter::TouchAdapter touch_adapter;
deskbot::device::CoreS3ImuDriver imu_driver;
deskbot::adapter::ImuAdapter imu_adapter;
deskbot::device::CoreS3CameraDriver camera_driver;
deskbot::vision::CameraVisionInput camera_vision;

namespace {

uint32_t last_face_render_ms = 0;
uint32_t last_camera_capture_ms = 0;
uint32_t last_body_motion_ms = 0;
bool body_motion_seen = false;
uint32_t last_recovery_trace_ms = 0;
deskbot::ghost::AutonomousAction last_autonomous_trace_action =
    deskbot::ghost::AutonomousAction::NONE;
uint32_t last_autonomous_trace_decision_ms = 0;
uint32_t last_autonomous_trace_decision_seq = 0;
uint32_t last_autonomous_lifecycle_seq = 0;
constexpr uint32_t kFaceRenderIntervalMs = 40;
constexpr uint32_t kCameraCaptureIntervalMs = 2000;

const char* autonomousActionName(deskbot::ghost::AutonomousAction action) {
  switch (action) {
    case deskbot::ghost::AutonomousAction::CURIOUS_LOOK:
      return "CURIOUS_LOOK";
    case deskbot::ghost::AutonomousAction::BORED_SCAN:
      return "BORED_SCAN";
    case deskbot::ghost::AutonomousAction::NONE:
    default:
      return "NONE";
  }
}

const char* autonomousLifecycleName(
    deskbot::ghost::AutonomousLifecycle lifecycle) {
  switch (lifecycle) {
    case deskbot::ghost::AutonomousLifecycle::START:
      return "START";
    case deskbot::ghost::AutonomousLifecycle::PAUSE:
      return "PAUSE";
    case deskbot::ghost::AutonomousLifecycle::RESUME:
      return "RESUME";
    case deskbot::ghost::AutonomousLifecycle::COMPLETE:
      return "COMPLETE";
    case deskbot::ghost::AutonomousLifecycle::CANCEL:
      return "CANCEL";
    case deskbot::ghost::AutonomousLifecycle::NONE:
    default:
      return "NONE";
  }
}

void traceAutonomousBehavior() {
  const auto& behavior = runtime.ghost().behaviorEngine();
  const auto action = behavior.autonomousAction();
  const uint32_t decision_ms = behavior.lastAutonomousDecisionMs();
  const uint32_t decision_seq = behavior.autonomousDecisionSeq();
  const uint32_t lifecycle_seq = behavior.autonomousLifecycleSeq();

  if (lifecycle_seq != last_autonomous_lifecycle_seq) {
    last_autonomous_lifecycle_seq = lifecycle_seq;
    const auto& heart = runtime.ghost().heart();
    Serial.printf("[BEHAVIOR][AUTO] %s %s curiosity=%.3f boredom=%.3f attention=%.3f\n",
                  autonomousLifecycleName(behavior.lastAutonomousLifecycle()),
                  autonomousActionName(action),
                  heart.curiosity,
                  heart.boredom,
                  heart.attention);
  }

  const bool action_changed = action != last_autonomous_trace_action;
  const bool decision_changed =
      decision_seq != last_autonomous_trace_decision_seq;
  if (!action_changed && !decision_changed) {
    return;
  }
  last_autonomous_trace_action = action;
  last_autonomous_trace_decision_ms = decision_ms;
  last_autonomous_trace_decision_seq = decision_seq;

  // NONE decisions have no lifecycle transition but are still real decisions.
  if (action == deskbot::ghost::AutonomousAction::NONE && decision_changed) {
    const auto& heart = runtime.ghost().heart();
    Serial.printf("[BEHAVIOR][AUTO] DECIDE NONE curiosity=%.3f boredom=%.3f attention=%.3f\n",
                  heart.curiosity,
                  heart.boredom,
                  heart.attention);
  }
}

void traceHeartRecovery() {
  auto& heart_engine = runtime.ghost().heartEngine();
  const uint32_t recovery_ms = heart_engine.lastRecoveryMs();
  if (recovery_ms == 0 || recovery_ms == last_recovery_trace_ms) {
    return;
  }
  last_recovery_trace_ms = recovery_ms;
  const auto& heart = runtime.ghost().heart();
  const auto& baseline = heart_engine.baseline();
  Serial.printf("[HEART][RECOVER] mood=%.3f->%.3f curiosity=%.3f->%.3f attention=%.3f->%.3f boredom=%.3f\n",
                heart.mood, baseline.mood,
                heart.curiosity, baseline.curiosity,
                heart.attention, baseline.attention,
                heart.boredom);
}

void traceHeartState(const char* cause) {
  const auto& heart = runtime.ghost().heart();
  Serial.printf("[HEART][CHANGE] %s impact=%.2f mood=%.3f affection=%.3f curiosity=%.3f boredom=%.3f sleepiness=%.3f attention=%.3f\n",
                cause,
                runtime.ghost().heartEngine().lastImpactScale(),
                heart.mood,
                heart.affection,
                heart.curiosity,
                heart.boredom,
                heart.sleepiness,
                heart.attention);
}

void traceVisionDelivery(const deskbot::nerve::SemanticNeuron& neuron) {
  auto& ghost = runtime.ghost();
  const auto& memory = ghost.memoryEngine().lastCandidate();
  const bool ghost_ok = ghost.lastNeuronType() == neuron.type &&
                        ghost.lastNeuronMs() == neuron.timestamp_ms;
  const bool heart_ok = ghost.heartEngine().lastEventType() == neuron.type &&
                        ghost.heartEngine().lastEventMs() == neuron.timestamp_ms;
  const bool memory_ok = memory.kind == deskbot::ghost::MemoryRecordKind::SEMANTIC_EVENT &&
                         memory.neuron.type == neuron.type &&
                         memory.neuron.timestamp_ms == neuron.timestamp_ms;
  const bool behavior_ok = ghost.behaviorEngine().lastReceivedType() == neuron.type &&
                           ghost.behaviorEngine().lastReceivedMs() == neuron.timestamp_ms;

  Serial.printf("[TRACE][VISION] Ghost=%s Heart=%s Memory=%s Behavior=%s\n",
                ghost_ok ? "YES" : "NO",
                heart_ok ? "YES" : "NO",
                memory_ok ? "YES" : "NO",
                behavior_ok ? "YES" : "NO");
}

void renderLivingFace(uint32_t now_ms) {
  if ((now_ms - last_face_render_ms) < kFaceRenderIntervalMs) {
    return;
  }
  last_face_render_ms = now_ms;

  const auto& micro = runtime.ghost().behaviorEngine().microBehavior();
  auto expression = deskbot::face::FaceRenderer::neutral();

  static_cast<deskbot::face::EyeShape&>(expression.left) = micro.left_shape;
  static_cast<deskbot::face::EyeShape&>(expression.right) = micro.right_shape;
  expression.spacing_scale = micro.eye_spacing_scale;
  expression.jitter_x = micro.jitter_x;
  expression.jitter_y = micro.jitter_y;
  expression.left.openness = micro.eye_openness + micro.left_eye_bias;
  expression.right.openness = micro.eye_openness + micro.right_eye_bias;
  expression.offset_x = micro.gaze_x;
  expression.offset_y = micro.gaze_y;

  face_renderer.render(expression, now_ms);
}

void pollTouch(uint32_t now_ms) {
  const auto sample = touch_driver.sample(now_ms);
  deskbot::nerve::SemanticNeuron neuron;
  if (touch_adapter.toNeuron(sample, neuron)) {
    runtime.emit(neuron);
    traceHeartState("TOUCH");
    Serial.printf("[SENSE][TOUCH] TOUCH x=%ld y=%ld\n",
                  static_cast<long>(neuron.payload.x),
                  static_cast<long>(neuron.payload.y));
  }
}

void pollImu(uint32_t now_ms) {
  const auto sample = imu_driver.sample(now_ms);
  deskbot::nerve::SemanticNeuron neuron;
  if (!imu_adapter.toNeuron(sample, neuron)) {
    return;
  }

  runtime.emit(neuron);
  if (neuron.type == deskbot::nerve::NeuronType::PICKED_UP ||
      neuron.type == deskbot::nerve::NeuronType::SHAKE) {
    last_body_motion_ms = now_ms;
    body_motion_seen = true;
    camera_vision.reset();
  }

  if (neuron.type == deskbot::nerve::NeuronType::PICKED_UP) {
    traceHeartState("PICKED_UP");
    Serial.printf("[SENSE][IMU] PICKED_UP motion=%.3f\n", neuron.payload.scalar);
  } else if (neuron.type == deskbot::nerve::NeuronType::SHAKE) {
    traceHeartState("SHAKE");
    Serial.printf("[SENSE][IMU] SHAKE motion=%.3f\n", neuron.payload.scalar);
  }
}

void captureCameraFrame(uint32_t now_ms, bool startup_probe) {
  const auto capture = camera_driver.captureOnce(now_ms, &camera_vision);
  const auto& vision = camera_vision.lastSummary();

  // Event-driven Serial policy:
  // - One startup READY line is useful.
  // - Normal periodic captures stay silent.
  // - Failures are always reported.
  if (capture.initialized && capture.captured && vision.valid) {
    if (startup_probe) {
      Serial.printf("[SENSE][CAMERA] Probe: READY frame=%ux%u bytes=%u luma=%u\n",
                    static_cast<unsigned>(vision.width),
                    static_cast<unsigned>(vision.height),
                    static_cast<unsigned>(vision.bytes),
                    static_cast<unsigned>(vision.average_luma));
    }
  } else if (capture.initialized) {
    Serial.println("[ERROR][CAMERA] CAPTURE_FAILED");
  } else {
    Serial.println("[ERROR][CAMERA] INIT_FAILED");
  }

  // Only dispatch after the camera has released I2C. Failed or known
  // self-motion captures invalidate the baseline and cannot leak stale events.
  const uint32_t delivered_ms = millis();
  const bool suppress = body_motion_seen && delivered_ms - last_body_motion_ms < 3000;
  if (!capture.captured || !capture.internal_i2c_restored || !vision.valid || suppress) {
    camera_vision.reset();
  } else {
    deskbot::nerve::SemanticNeuron neuron;
    if (camera_vision.takeNeuron(neuron)) {
      runtime.tick(delivered_ms);
      runtime.emit(neuron);
      traceVisionDelivery(neuron);
      const char* name = neuron.type == deskbot::nerve::NeuronType::MOTION_DETECTED
          ? "MOTION_DETECTED" : (neuron.type == deskbot::nerve::NeuronType::BRIGHTER ? "BRIGHTER" : "DARKER");
      traceHeartState(name);
      Serial.printf("[NERVE][VISION] %s strength=%.3f -> Ghost/Heart/Memory/Behavior\n",
                    name, neuron.payload.scalar);
    }
  }

  if (!capture.internal_i2c_restored) {
    Serial.println("[ERROR][CAMERA] Internal I2C restore failed");
  }
}

void pollCamera(uint32_t now_ms) {
  if ((now_ms - last_camera_capture_ms) < kCameraCaptureIntervalMs) {
    return;
  }
  last_camera_capture_ms = now_ms;
  captureCameraFrame(now_ms, false);
}

}  // namespace

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  Serial.begin(115200);
  delay(300);

  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextSize(2);
  M5.Display.drawString("CoreS3 BASE", M5.Display.width() / 2, M5.Display.height() / 2 - 16);
  M5.Display.setTextSize(1);
  M5.Display.drawString("NERVE / GHOST READY", M5.Display.width() / 2, M5.Display.height() / 2 + 18);

  Serial.println("[BOOT] M5Stack Desktop Companion");
  Serial.println("[BASE] CoreS3 clean base started");
  Serial.println("[BASE] No Stack-chan / AI_StackChan_Ex / stack-chan-ko / RoboEyes");

  const bool nerve_ready = runtime.begin(millis());
  Serial.printf("[NERVE] Synapse bindings: %u\n", static_cast<unsigned>(runtime.synapse().bindingCount()));
  Serial.printf("[NERVE] Runtime: %s\n", nerve_ready ? "READY" : "ERROR");

  const auto& heart = runtime.ghost().heart();
  const auto& heart_engine = runtime.ghost().heartEngine();
  Serial.printf("[HEART] mood=%.2f affection=%.2f curiosity=%.2f\n",
                heart.mood, heart.affection, heart.curiosity);
  Serial.printf("[HEART] boredom=%.2f sleepiness=%.2f attention=%.2f\n",
                heart.boredom, heart.sleepiness, heart.attention);

  if (heart_engine.primarySnapshotLoaded()) {
    Serial.println("[HEART][NVS] Existing primary snapshot loaded");
    Serial.printf("[HEART][RESTORE] LOCK20 field plan: %s\n",
                  heart_engine.restorePending() ? "PENDING_STEP9_INPUTS" : "READY");
  } else if (heart_engine.firstBootInitialized()) {
    Serial.printf("[HEART][NVS] First boot snapshot initialized: %s\n",
                  heart_engine.primarySaveOk() ? "OK" : "ERROR");
  } else {
    Serial.println("[HEART][NVS] Primary snapshot state unknown");
  }

  // Camera now has a reusable Device Driver -> Vision frame boundary. We keep
  // each capture transaction self-contained until repeated real-device tests
  // prove that camera use can coexist with the standing Touch/IMU senses.
  captureCameraFrame(millis(), true);
  last_camera_capture_ms = millis();

  touch_driver.begin();
  Serial.printf("[SENSE][TOUCH] Device driver: %s\n",
                M5.Touch.isEnabled() ? "READY" : "UNAVAILABLE");

  imu_driver.begin();
  imu_adapter.begin(millis());
  Serial.printf("[SENSE][IMU] Device driver: %s\n",
                imu_driver.available() ? "READY" : "UNAVAILABLE");

  face_renderer.begin(M5.Display);
  runtime.tick(millis());
  renderLivingFace(millis());
  Serial.println("[DESKROBO] Standalone Heart/Time -> Behavior -> Face life loop started");
}

void loop() {
  M5.update();

  const uint32_t now_ms = millis();
  runtime.tick(now_ms);
  pollTouch(now_ms);
  pollImu(now_ms);
  pollCamera(now_ms);
  // Capture is blocking; use fresh time so a newly delivered event is not
  // compared against a tick timestamp from before it occurred.
  const uint32_t body_now_ms = millis();
  runtime.tick(body_now_ms);
  traceHeartRecovery();
  traceAutonomousBehavior();
  renderLivingFace(body_now_ms);

  delay(10);
}

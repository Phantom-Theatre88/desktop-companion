#include <M5Unified.h>
#include <M5GFX.h>

#include "src/adapter/ImuAdapter.h"
#include "src/adapter/TouchAdapter.h"
#include "src/core/DesktopCompanionRuntime.h"
#include "src/device/CoreS3ImuDriver.h"
#include "src/device/CoreS3TouchDriver.h"
#include "src/face/FaceRenderer.h"

deskbot::core::DesktopCompanionRuntime runtime;
deskbot::face::FaceRenderer face_renderer;
deskbot::device::CoreS3TouchDriver touch_driver;
deskbot::adapter::TouchAdapter touch_adapter;
deskbot::device::CoreS3ImuDriver imu_driver;
deskbot::adapter::ImuAdapter imu_adapter;

namespace {

uint32_t last_face_render_ms = 0;
constexpr uint32_t kFaceRenderIntervalMs = 40;

void renderLivingFace(uint32_t now_ms) {
  if ((now_ms - last_face_render_ms) < kFaceRenderIntervalMs) {
    return;
  }
  last_face_render_ms = now_ms;

  const auto& micro = runtime.ghost().behaviorEngine().microBehavior();
  auto expression = deskbot::face::FaceRenderer::neutral();

  expression.left.openness = micro.eye_openness + micro.left_eye_bias;
  expression.right.openness = micro.eye_openness + micro.right_eye_bias;
  expression.offset_x = micro.gaze_x;
  expression.offset_y = micro.gaze_y;

  face_renderer.render(expression);
}

void pollTouch(uint32_t now_ms) {
  const auto sample = touch_driver.sample(now_ms);
  deskbot::nerve::SemanticNeuron neuron;
  if (touch_adapter.toNeuron(sample, neuron)) {
    runtime.emit(neuron);
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

  if (neuron.type == deskbot::nerve::NeuronType::PICKED_UP) {
    Serial.printf("[SENSE][IMU] PICKED_UP motion=%.3f\n", neuron.payload.scalar);
  } else if (neuron.type == deskbot::nerve::NeuronType::SHAKE) {
    Serial.printf("[SENSE][IMU] SHAKE motion=%.3f\n", neuron.payload.scalar);
  }
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
  pollTouch(now_ms);
  pollImu(now_ms);
  runtime.tick(now_ms);
  renderLivingFace(now_ms);

  delay(10);
}

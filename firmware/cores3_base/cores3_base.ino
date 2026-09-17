#include <M5Unified.h>
#include <M5GFX.h>

#include "src/core/DesktopCompanionRuntime.h"
#include "src/face/FaceRenderer.h"

deskbot::core::DesktopCompanionRuntime runtime;
deskbot::face::FaceRenderer face_renderer;

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

  face_renderer.begin(M5.Display);
  face_renderer.render(deskbot::face::FaceRenderer::neutral());
  Serial.println("[FACE] Neutral expression rendered");
}

void loop() {
  M5.update();
  runtime.tick(millis());
  delay(10);
}

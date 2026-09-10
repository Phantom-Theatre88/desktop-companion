#include <M5Unified.h>
#include <M5GFX.h>

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
  M5.Display.drawString("STEP 0 / CLEAN START", M5.Display.width() / 2, M5.Display.height() / 2 + 18);

  Serial.println("[BOOT] M5Stack Desktop Companion");
  Serial.println("[STEP0] CoreS3 clean base started");
  Serial.println("[STEP0] No Stack-chan / AI_StackChan_Ex / stack-chan-ko / RoboEyes");
}

void loop() {
  M5.update();
  delay(10);
}

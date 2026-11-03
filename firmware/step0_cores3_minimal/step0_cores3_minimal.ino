#include <M5Unified.h>
#include <M5GFX.h>

void setup() {
  M5.begin();
  Serial.begin(115200);

  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextSize(2);
  M5.Display.setCursor(12, 16);
  M5.Display.println("CoreS3 Step 0");
  M5.Display.setTextSize(1);
  M5.Display.setCursor(12, 52);
  M5.Display.println("Minimal base ready");

  Serial.println("[STEP0] CoreS3 minimal base started");
}

void loop() {
  M5.update();
}

#pragma once

#include <M5GFX.h>

namespace deskbot {
namespace face {

struct EyeParams {
  float openness = 1.0f;      // 0.0 closed .. 1.0 fully open
  float width_scale = 1.0f;   // base eye width multiplier
  float height_scale = 1.0f;  // base eye height multiplier
  float tilt = 0.0f;          // -1.0 droop .. +1.0 angry/upward inner edge
};

struct ExpressionParams {
  EyeParams left{};
  EyeParams right{};
  float offset_x = 0.0f;      // -1.0 .. +1.0, whole-face horizontal shift
  float offset_y = 0.0f;      // -1.0 .. +1.0, whole-face vertical shift
  uint32_t eye_color = TFT_CYAN;
};

class FaceRenderer {
 public:
  void begin(M5GFX& display);
  void render(const ExpressionParams& expression);

  static ExpressionParams neutral();
  static ExpressionParams sleepy();
  static ExpressionParams angry();
  static ExpressionParams sad();
  static ExpressionParams surprised();
  static ExpressionParams dead();

 private:
  void drawEye(int16_t cx, int16_t cy, const EyeParams& eye, bool is_left, uint32_t color);
  static float clamp01(float value);
  static float clampSigned(float value);

  M5GFX* display_ = nullptr;
  M5Canvas* canvas_ = nullptr;
};

}  // namespace face
}  // namespace deskbot

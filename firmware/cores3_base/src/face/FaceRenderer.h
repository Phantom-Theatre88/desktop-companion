#pragma once

#include <M5GFX.h>
#include "FaceShape.h"

namespace deskbot {
namespace face {

struct EyeParams : EyeShape {
  float openness = 1.0f;      // 0.0 closed .. 1.0 fully open
};

struct ExpressionParams {
  EyeParams left{};
  EyeParams right{};
  float offset_x = 0.0f;      // -1.0 .. +1.0, whole-face horizontal shift
  float offset_y = 0.0f;      // -1.0 .. +1.0, whole-face vertical shift
  float spacing_scale = 1.0f; // center separation relative to the neutral face
  float jitter_x = 0.0f;      // -1..1, composed body displacement
  float jitter_y = 0.0f;
  float mouth_open = 0.0f;     // 0 hidden .. 1 fully open composed mouth
  VisualProp prop = VisualProp::NONE;
  float prop_progress = 0.0f;   // 0..1 animation progress supplied by Behavior
  VisualEffect effect = VisualEffect::NONE;
  float effect_progress = 0.0f; // 0..1 animation progress supplied by Behavior/Reflex
  float effect_amount = 1.0f;   // 0..1 intensity, meaning decided upstream
  bool sleep_zzz = false;
  float sleep_zzz_phase = 0.0f; // 0..1 repeating drift phase from Behavior
  uint32_t eye_color = TFT_CYAN;
};

class FaceRenderer {
 public:
  void begin(M5GFX& display);
  void render(const ExpressionParams& expression);
  void render(const ExpressionParams& expression, uint32_t now_ms);

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

  ShapeTransition transition_{};
  M5GFX* display_ = nullptr;
  M5Canvas* canvas_ = nullptr;
};

}  // namespace face
}  // namespace deskbot

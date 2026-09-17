#include "FaceRenderer.h"

namespace deskbot {
namespace face {

void FaceRenderer::begin(M5GFX& display) {
  display_ = &display;

  if (canvas_ == nullptr) {
    canvas_ = new M5Canvas(&display);
    canvas_->setColorDepth(16);
    canvas_->createSprite(display.width(), display.height());
  }
}

void FaceRenderer::render(const ExpressionParams& expression) {
  if (display_ == nullptr || canvas_ == nullptr) {
    return;
  }

  // Draw the complete frame offscreen, then push it in one operation.
  // This prevents the visible black-clear -> eye-redraw flashing that occurs
  // when the physical LCD is cleared on every life-loop frame.
  canvas_->fillSprite(TFT_BLACK);

  const int16_t w = display_->width();
  const int16_t h = display_->height();
  const int16_t base_y = h / 2;
  const int16_t eye_gap = w / 6;
  const int16_t shift_x = static_cast<int16_t>(clampSigned(expression.offset_x) * (w * 0.10f));
  const int16_t shift_y = static_cast<int16_t>(clampSigned(expression.offset_y) * (h * 0.10f));

  drawEye(w / 2 - eye_gap + shift_x,
          base_y + shift_y,
          expression.left,
          true,
          expression.eye_color);
  drawEye(w / 2 + eye_gap + shift_x,
          base_y + shift_y,
          expression.right,
          false,
          expression.eye_color);

  canvas_->pushSprite(0, 0);
}

void FaceRenderer::drawEye(int16_t cx,
                           int16_t cy,
                           const EyeParams& eye,
                           bool is_left,
                           uint32_t color) {
  if (display_ == nullptr || canvas_ == nullptr) {
    return;
  }

  const float open = clamp01(eye.openness);
  const float width_scale = eye.width_scale < 0.2f ? 0.2f : eye.width_scale;
  const float height_scale = eye.height_scale < 0.2f ? 0.2f : eye.height_scale;
  const float tilt = clampSigned(eye.tilt);

  const int16_t base_w = display_->width() / 5;
  const int16_t base_h = display_->height() / 4;
  const int16_t ew = static_cast<int16_t>(base_w * width_scale);
  const int16_t eh = static_cast<int16_t>(base_h * height_scale * (0.12f + 0.88f * open));
  const int16_t x = cx - ew / 2;
  const int16_t y = cy - eh / 2;
  const int16_t radius = eh / 2;

  canvas_->fillRoundRect(x, y, ew, eh, radius, color);

  // Shape the eyelid by cutting the upper edge with the black background.
  // Positive tilt makes the inner corner lower: a sharper/angrier look.
  // Negative tilt makes the inner corner higher: a softer/sadder look.
  const int16_t cut = static_cast<int16_t>(tilt * (eh * 0.45f));
  if (cut != 0) {
    const int16_t left_top = y;
    const int16_t right_top = y;

    if (is_left) {
      canvas_->fillTriangle(x,
                            left_top,
                            x + ew,
                            right_top,
                            x + ew,
                            right_top + cut,
                            TFT_BLACK);
    } else {
      canvas_->fillTriangle(x,
                            left_top,
                            x + ew,
                            right_top,
                            x,
                            left_top + cut,
                            TFT_BLACK);
    }
  }
}

ExpressionParams FaceRenderer::neutral() {
  ExpressionParams p;
  p.left.openness = 0.90f;
  p.right.openness = 0.90f;
  p.left.width_scale = 1.00f;
  p.right.width_scale = 1.00f;
  p.left.height_scale = 1.00f;
  p.right.height_scale = 1.00f;
  return p;
}

ExpressionParams FaceRenderer::sleepy() {
  ExpressionParams p = neutral();
  p.left.openness = 0.18f;
  p.right.openness = 0.18f;
  p.left.width_scale = 1.15f;
  p.right.width_scale = 1.15f;
  return p;
}

ExpressionParams FaceRenderer::angry() {
  ExpressionParams p = neutral();
  p.left.openness = 0.68f;
  p.right.openness = 0.68f;
  p.left.tilt = 0.85f;
  p.right.tilt = 0.85f;
  return p;
}

ExpressionParams FaceRenderer::sad() {
  ExpressionParams p = neutral();
  p.left.openness = 0.72f;
  p.right.openness = 0.72f;
  p.left.tilt = -0.75f;
  p.right.tilt = -0.75f;
  return p;
}

ExpressionParams FaceRenderer::surprised() {
  ExpressionParams p = neutral();
  p.left.openness = 1.0f;
  p.right.openness = 1.0f;
  p.left.width_scale = 0.82f;
  p.right.width_scale = 0.82f;
  p.left.height_scale = 1.20f;
  p.right.height_scale = 1.20f;
  return p;
}

ExpressionParams FaceRenderer::dead() {
  ExpressionParams p = sleepy();
  p.left.openness = 0.10f;
  p.right.openness = 0.10f;
  return p;
}

float FaceRenderer::clamp01(float value) {
  if (value < 0.0f) {
    return 0.0f;
  }
  if (value > 1.0f) {
    return 1.0f;
  }
  return value;
}

float FaceRenderer::clampSigned(float value) {
  if (value < -1.0f) {
    return -1.0f;
  }
  if (value > 1.0f) {
    return 1.0f;
  }
  return value;
}

}  // namespace face
}  // namespace deskbot

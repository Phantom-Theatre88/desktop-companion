#include "FaceRenderer.h"

namespace deskbot {
namespace face {

void FaceRenderer::begin(M5GFX& display) {
  display_ = &display;
  transition_.reset();

  if (canvas_ == nullptr) {
    canvas_ = new M5Canvas(&display);
    canvas_->setColorDepth(16);
    canvas_->createSprite(display.width(), display.height());
  }
}

void FaceRenderer::render(const ExpressionParams& expression) {
  render(expression, millis());
}

void FaceRenderer::render(const ExpressionParams& target, uint32_t now_ms) {
  transition_.update(target.left, target.right, target.spacing_scale, now_ms);
  ExpressionParams expression = target;
  static_cast<EyeShape&>(expression.left) = transition_.left();
  static_cast<EyeShape&>(expression.right) = transition_.right();
  if (display_ == nullptr || canvas_ == nullptr) {
    return;
  }

  // Build the whole frame offscreen and push it atomically. This keeps the
  // standalone life loop free of visible clear/redraw flicker.
  canvas_->fillSprite(TFT_BLACK);

  const int16_t w = display_->width();
  const int16_t h = display_->height();

  // Reference-face DNA: the eyes are the dominant feature. Keep them large and
  // relatively far apart instead of rendering two small circular dots.
  const int16_t base_y = static_cast<int16_t>(h * 0.47f);
  const int16_t eye_gap = static_cast<int16_t>(static_cast<int16_t>(w * 0.215f) * transition_.spacing());
  int16_t shift_x = static_cast<int16_t>(clampSigned(expression.offset_x) * (w * 0.075f) + clampSigned(expression.jitter_x) * (w * 0.025f));
  const int16_t shift_y = static_cast<int16_t>(clampSigned(expression.offset_y) * (h * 0.070f) + clampSigned(expression.jitter_y) * (h * 0.025f));

  // Constrain the combined gaze + transient shift using both actual widths.
  // Odd pixel widths need a ceil half-width on the right edge.
  const int16_t base_width = static_cast<int16_t>(w * 0.275f);
  const int16_t left_width = static_cast<int16_t>(base_width * expression.left.width_scale);
  const int16_t right_width = static_cast<int16_t>(base_width * expression.right.width_scale);
  const int16_t min_shift = left_width / 2 - (w / 2 - eye_gap);
  const int16_t max_shift = w - (w / 2 + eye_gap) - (right_width - right_width / 2);
  if (shift_x < min_shift) shift_x = min_shift;
  if (shift_x > max_shift) shift_x = max_shift;

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

  // Mouth is intentionally absent in the neutral face. The composed body
  // output may request a transient mouth from Reflex or Heart/Behavior.
  const float mouth_open = clamp01(expression.mouth_open);
  if (mouth_open > 0.04f) {
    const int16_t mouth_cx = w / 2;
    const int16_t mouth_cy = static_cast<int16_t>(h * 0.72f);
    const int16_t outer_rx = static_cast<int16_t>(w * 0.035f);
    const int16_t outer_ry = static_cast<int16_t>(
        h * (0.018f + 0.035f * mouth_open));
    const int16_t inner_rx = outer_rx > 3 ? outer_rx - 3 : 1;
    const int16_t inner_ry = outer_ry > 3 ? outer_ry - 3 : 1;
    canvas_->fillEllipse(mouth_cx, mouth_cy, outer_rx, outer_ry,
                         expression.eye_color);
    canvas_->fillEllipse(mouth_cx, mouth_cy, inner_rx, inner_ry, TFT_BLACK);
  }

  // Props are transient Behavior results, never part of the neutral face.
  if (expression.prop == VisualProp::COFFEE_CUP) {
    const float p = clamp01(expression.prop_progress);
    const float approach = sinf(p * 3.14159265f);
    const int16_t cup_x = static_cast<int16_t>(
        w * (0.78f - 0.22f * approach));
    const int16_t cup_y = static_cast<int16_t>(
        h * (0.79f - 0.08f * approach));
    const int16_t cup_w = static_cast<int16_t>(w * 0.105f);
    const int16_t cup_h = static_cast<int16_t>(h * 0.090f);

    canvas_->drawRoundRect(cup_x - cup_w / 2,
                           cup_y - cup_h / 2,
                           cup_w,
                           cup_h,
                           5,
                           expression.eye_color);
    canvas_->drawCircle(cup_x + cup_w / 2 + 4,
                        cup_y,
                        static_cast<int16_t>(cup_h * 0.28f),
                        expression.eye_color);

    const int16_t steam_h = static_cast<int16_t>(h * 0.035f);
    canvas_->drawLine(cup_x - 6,
                      cup_y - cup_h / 2 - 4,
                      cup_x - 3,
                      cup_y - cup_h / 2 - steam_h,
                      expression.eye_color);
    canvas_->drawLine(cup_x + 5,
                      cup_y - cup_h / 2 - 3,
                      cup_x + 8,
                      cup_y - cup_h / 2 - steam_h + 2,
                      expression.eye_color);
  }

  // Deep-sleep effect: a few Zs drift upward and fade by position.
  // This is explanatory Behavior decoration, not the primary emotional face.
  if (expression.sleep_zzz) {
    const float phase = clamp01(expression.sleep_zzz_phase);
    canvas_->setTextDatum(middle_center);
    for (int i = 0; i < 3; ++i) {
      float p = phase + static_cast<float>(i) * 0.33f;
      if (p >= 1.0f) p -= 1.0f;
      const int16_t zx = static_cast<int16_t>(w * (0.73f + 0.08f * p));
      const int16_t zy = static_cast<int16_t>(h * (0.63f - 0.30f * p));
      canvas_->setTextSize(i == 0 ? 2 : 1);
      canvas_->setTextColor(expression.eye_color, TFT_BLACK);
      canvas_->drawString("Z", zx, zy);
    }
  }

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

  // Larger than the old round-dot eyes. The reference uses a soft trapezoid:
  // broad top, rounded lower edge, and small clipped upper corners.
  const int16_t base_w = static_cast<int16_t>(display_->width() * 0.275f);
  const int16_t base_h = static_cast<int16_t>(display_->height() * 0.285f);
  const int16_t ew = static_cast<int16_t>(base_w * width_scale);

  // When fully closed, render a proper eyelid line rather than a tiny pill.
  if (open <= 0.055f) {
    const int16_t line_h = 5;
    const int16_t x = cx - ew / 2;
    const int16_t y = cy - line_h / 2;
    canvas_->fillRoundRect(x, y, ew, line_h, line_h / 2, color);
    return;
  }

  // Keep the eye substantial through normal expressions, but still allow a
  // continuous squash into a blink.
  const float open_height = 0.16f + (0.84f * open);
  const int16_t eh = static_cast<int16_t>(base_h * height_scale * open_height);
  const int16_t x = cx - ew / 2;
  const int16_t y = cy - eh / 2;

  int16_t radius = static_cast<int16_t>(eh * 0.32f);
  const int16_t radius_limit = ew / 5;
  if (radius > radius_limit) {
    radius = radius_limit;
  }
  if (radius < 3) {
    radius = 3;
  }

  radius = static_cast<int16_t>(radius * eye.radius_scale);
  const int16_t max_radius = (ew < eh ? ew : eh) / 2;
  if (radius > max_radius) radius = max_radius;
  canvas_->fillRoundRect(x, y, ew, eh, radius, color);

  // Reference silhouette: shave the two upper corners into short diagonals.
  // This is what prevents the neutral face from reading as two circles.
  int16_t corner_cut = static_cast<int16_t>(ew * 0.085f);
  if (corner_cut < 4) {
    corner_cut = 4;
  }
  const int16_t corner_drop = static_cast<int16_t>(eh * 0.18f);

  canvas_->fillTriangle(x,
                        y,
                        x + corner_cut,
                        y,
                        x,
                        y + corner_drop,
                        TFT_BLACK);
  canvas_->fillTriangle(x + ew,
                        y,
                        x + ew - corner_cut,
                        y,
                        x + ew,
                        y + corner_drop,
                        TFT_BLACK);

  // Expression tilt deforms the upper eyelid while keeping the same base eye
  // family. Positive tilt sharpens the inner edge, negative tilt softens it.
  const int16_t tilt_cut = static_cast<int16_t>(tilt * (eh * 0.38f));
  if (tilt_cut != 0) {
    if (is_left) {
      if (tilt_cut > 0) {
        canvas_->fillTriangle(x,
                              y,
                              x + ew,
                              y,
                              x + ew,
                              y + tilt_cut,
                              TFT_BLACK);
      } else {
        canvas_->fillTriangle(x,
                              y,
                              x + ew,
                              y,
                              x,
                              y - tilt_cut,
                              TFT_BLACK);
      }
    } else {
      if (tilt_cut > 0) {
        canvas_->fillTriangle(x,
                              y,
                              x + ew,
                              y,
                              x,
                              y + tilt_cut,
                              TFT_BLACK);
      } else {
        canvas_->fillTriangle(x,
                              y,
                              x + ew,
                              y,
                              x + ew,
                              y - tilt_cut,
                              TFT_BLACK);
      }
    }
  }
  // Masks stay inside this eye's rectangle. Independent left/right controls
  // compose with the existing mirrored upper-lid slope (tilt).
  const int16_t upper = static_cast<int16_t>(eh * eye.upper_lid);
  const int16_t lower = static_cast<int16_t>(eh * eye.lower_lid);
  if (upper > 0) canvas_->fillRect(x, y, ew, upper, TFT_BLACK);
  if (lower > 0) canvas_->fillRect(x, y + eh - lower, ew, lower, TFT_BLACK);
}

ExpressionParams FaceRenderer::neutral() {
  ExpressionParams p;
  p.left.openness = 0.88f;
  p.right.openness = 0.88f;
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
  p.left.width_scale = 1.04f;
  p.right.width_scale = 1.04f;
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
  p.left.width_scale = 0.88f;
  p.right.width_scale = 0.88f;
  p.left.height_scale = 1.08f;
  p.right.height_scale = 1.08f;
  return p;
}

ExpressionParams FaceRenderer::dead() {
  ExpressionParams p = sleepy();
  p.left.openness = 0.05f;
  p.right.openness = 0.05f;
  return p;
}

float FaceRenderer::clamp01(float value) {
  if (!isfinite(value)) return 0.0f;
  if (value < 0.0f) {
    return 0.0f;
  }
  if (value > 1.0f) {
    return 1.0f;
  }
  return value;
}

float FaceRenderer::clampSigned(float value) {
  if (!isfinite(value)) return 0.0f;
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

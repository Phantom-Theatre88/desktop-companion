#pragma once

#include <stdint.h>
#include <math.h>

namespace deskbot {
namespace face {

enum class VisualProp : uint8_t {
  NONE = 0,
  COFFEE_CUP,
};

// Body geometry only; no moods, random motion, or event selection.
struct EyeShape {
  float width_scale = 1.0f;        // 0.65..1.20 of existing neutral width
  float height_scale = 1.0f;       // 0.60..1.25 of existing neutral height
  float radius_scale = 1.0f;       // 0..2 of existing neutral corner radius
  float upper_lid = 0.0f;          // 0..1 coverage of eye height
  float tilt = 0.0f;               // -1..1 mirrored upper-lid slope
  float lower_lid = 0.0f;          // 0..1 coverage of eye height
};

inline float finiteClamp(float v, float low, float high, float fallback) {
  if (!isfinite(v)) return fallback;
  return v < low ? low : (v > high ? high : v);
}

inline EyeShape boundedShape(EyeShape p) {
  p.width_scale = finiteClamp(p.width_scale, 0.65f, 1.20f, 1.0f);
  p.height_scale = finiteClamp(p.height_scale, 0.60f, 1.25f, 1.0f);
  p.radius_scale = finiteClamp(p.radius_scale, 0.0f, 2.0f, 1.0f);
  p.tilt = finiteClamp(p.tilt, -1.0f, 1.0f, 0.0f);
  p.upper_lid = finiteClamp(p.upper_lid, 0.0f, 1.0f, 0.0f);
  p.lower_lid = finiteClamp(p.lower_lid, 0.0f, 1.0f, 0.0f);
  return p;
}

// Exponential tracking uses elapsed time, not frames. Blink and transient
// displacement bypass this filter so a short closure/shake is not swallowed.
class ShapeTransition {
 public:
  void reset() { *this = ShapeTransition{}; }
  void update(const EyeShape& left, const EyeShape& right, float spacing,
              uint32_t now_ms) {
    const EyeShape l = boundedShape(left), r = boundedShape(right);
    const float s = finiteClamp(spacing, 0.85f, 1.12f, 1.0f);
    if (!initialized_) {
      left_ = l; right_ = r; spacing_ = s; initialized_ = true;
    } else {
      const float a = 1.0f - expf(-static_cast<float>(now_ms - last_ms_) / 85.0f);
      approach(left_, l, a); approach(right_, r, a);
      spacing_ += (s - spacing_) * a;
    }
    last_ms_ = now_ms;
  }
  const EyeShape& left() const { return left_; }
  const EyeShape& right() const { return right_; }
  float spacing() const { return spacing_; }
 private:
  static void approach(EyeShape& p, const EyeShape& t, float a) {
    p.width_scale += (t.width_scale - p.width_scale) * a;
    p.height_scale += (t.height_scale - p.height_scale) * a;
    p.radius_scale += (t.radius_scale - p.radius_scale) * a;
    p.tilt += (t.tilt - p.tilt) * a;
    p.upper_lid += (t.upper_lid - p.upper_lid) * a;
    p.lower_lid += (t.lower_lid - p.lower_lid) * a;
  }
  EyeShape left_{}, right_{};
  float spacing_ = 1.0f;
  uint32_t last_ms_ = 0;
  bool initialized_ = false;
};

}  // namespace face
}  // namespace deskbot

#pragma once

#include <Arduino.h>

#include "../face/FaceRenderer.h"
#include "../ghost/BehaviorEngine.h"
#include "../reflex/ReflexLayer.h"

namespace deskbot {
namespace body {

// Final body-output boundary.
// Heart/Behavior supplies the continuous inner-state stream.
// Reflex supplies immediate sensor-driven overlays.
// FaceRenderer only draws the already-composed result.
class BodyOutputComposer {
 public:
  void begin(uint32_t now_ms);
  void onReflexIntent(const reflex::ReflexIntent& intent);

  face::ExpressionParams compose(
      const ghost::MicroBehaviorFrame& behavior,
      uint32_t now_ms) const;

 private:
  static bool isVisualCause(nerve::NeuronType type);
  static float clamp01(float value);
  static float clampSigned(float value);
  static float larger(float a, float b);

  void applyDirectReflex(face::ExpressionParams& expression,
                         const reflex::ReflexIntent& intent,
                         uint32_t now_ms) const;
  void applyVisualReflex(face::ExpressionParams& expression,
                         const reflex::ReflexIntent& intent,
                         uint32_t now_ms) const;

  reflex::ReflexIntent direct_reflex_{};
  reflex::ReflexIntent visual_reflex_{};
  bool have_direct_reflex_ = false;
  bool have_visual_reflex_ = false;
};

}  // namespace body
}  // namespace deskbot

#pragma once

#include <Arduino.h>

#include "../face/FaceRenderer.h"
#include "../ghost/BehaviorEngine.h"
#include "../reflex/ReflexLayer.h"

namespace deskbot {
namespace body {

struct BodyFrame {
  face::ExpressionParams face{};
  float neck_yaw = 0.0f;
  float neck_pitch = 0.0f;
};

// Final body-output boundary.
// Heart/Behavior supplies the continuous inner-state stream.
// Reflex supplies immediate sensor-driven overlays.
// Renderers/drivers only consume the already-composed result.
class BodyOutputComposer {
 public:
  enum class ArbitrationResult : uint8_t {
    ACCEPTED = 0,
    ACCEPTED_VISUAL,
    IGNORED_LOWER_PRIORITY,
  };

  void begin(uint32_t now_ms);
  ArbitrationResult onReflexIntent(const reflex::ReflexIntent& intent);

  BodyFrame compose(
      const ghost::MicroBehaviorFrame& behavior,
      uint32_t now_ms) const;

 private:
  enum class ReflexPriority : uint8_t {
    NONE = 0,
    VISUAL = 1,
    INTERPERSONAL = 2,
    STRONG = 3,
  };

  static bool isVisualCause(nerve::NeuronType type);
  static ReflexPriority priorityOf(const reflex::ReflexIntent& intent);
  static bool directIntentActiveAt(const reflex::ReflexIntent& intent,
                                   uint32_t now_ms);
  static float clamp01(float value);
  static float clampSigned(float value);
  static float larger(float a, float b);

  void applyDirectReflex(BodyFrame& body,
                         const reflex::ReflexIntent& intent,
                         uint32_t now_ms) const;
  void applyVisualReflex(BodyFrame& body,
                         const reflex::ReflexIntent& intent,
                         uint32_t now_ms) const;

  reflex::ReflexIntent direct_reflex_{};
  reflex::ReflexIntent visual_reflex_{};
  bool have_direct_reflex_ = false;
  bool have_visual_reflex_ = false;
};

}  // namespace body
}  // namespace deskbot

#include "BodyOutputComposer.h"

#include <math.h>

namespace deskbot {
namespace body {
namespace {

// Existing response timings are preserved during the architecture repair.
// They remain implementation tuning values, not personality LOCK values.
constexpr uint32_t kTouchResponseMs = 500;
constexpr uint32_t kWakeWordResponseMs = 950;
constexpr uint32_t kLiftStartResponseMs = 700;
constexpr uint32_t kLiftStartMouthMs = 350;
constexpr uint32_t kShakeResponseMs = 650;
constexpr uint32_t kVisualResponseMs = 1500;
constexpr uint32_t kMotionAttackMs = 120;
constexpr uint32_t kMotionHoldMs = 300;
constexpr uint32_t kMotionReleaseMs =
    kVisualResponseMs - kMotionAttackMs - kMotionHoldMs;
constexpr float kMotionDirectionDeadzone = 0.12f;

}  // namespace

void BodyOutputComposer::begin(uint32_t now_ms) {
  (void)now_ms;
  direct_reflex_ = reflex::ReflexIntent{};
  visual_reflex_ = reflex::ReflexIntent{};
  have_direct_reflex_ = false;
  have_visual_reflex_ = false;
}

BodyOutputComposer::ArbitrationResult BodyOutputComposer::onReflexIntent(
    const reflex::ReflexIntent& intent) {
  if (isVisualCause(intent.cause)) {
    visual_reflex_ = intent;
    have_visual_reflex_ = true;
    return ArbitrationResult::ACCEPTED_VISUAL;
  }

  // LOCK 15 / 36:
  // do not let a later low-priority body event overwrite a still-relevant
  // stronger reflex. Strong/safety reflex > interpersonal response.
  if (!have_direct_reflex_ ||
      !directIntentActiveAt(direct_reflex_, intent.created_ms) ||
      priorityOf(intent) >= priorityOf(direct_reflex_)) {
    direct_reflex_ = intent;
    have_direct_reflex_ = true;
    return ArbitrationResult::ACCEPTED;
  }

  return ArbitrationResult::IGNORED_LOWER_PRIORITY;
}

BodyFrame BodyOutputComposer::compose(
    const ghost::MicroBehaviorFrame& behavior,
    uint32_t now_ms) const {
  // Heart/Behavior is the continuous base layer.
  BodyFrame body;
  auto& expression = body.face;
  expression = face::FaceRenderer::neutral();
  static_cast<face::EyeShape&>(expression.left) = behavior.left_shape;
  static_cast<face::EyeShape&>(expression.right) = behavior.right_shape;
  expression.spacing_scale = behavior.eye_spacing_scale;
  expression.jitter_x = behavior.jitter_x;
  expression.jitter_y = behavior.jitter_y;
  expression.mouth_open = behavior.mouth_open;
  expression.prop = behavior.prop;
  expression.prop_progress = behavior.prop_progress;
  expression.effect = behavior.effect;
  expression.effect_progress = behavior.effect_progress;
  expression.effect_amount = behavior.effect_amount;
  expression.sleep_zzz = behavior.sleep_zzz;
  expression.sleep_zzz_phase = behavior.sleep_zzz_phase;
  expression.left.openness =
      clamp01(behavior.eye_openness + behavior.left_eye_bias);
  expression.right.openness =
      clamp01(behavior.eye_openness + behavior.right_eye_bias);
  expression.offset_x = clampSigned(behavior.gaze_x);
  expression.offset_y = clampSigned(behavior.gaze_y);
  body.neck_yaw = clampSigned(behavior.neck_yaw);
  body.neck_pitch = clampSigned(behavior.neck_pitch);

  // Strong/direct body reflexes have output priority over low-level visual
  // reflexes. The underlying Heart/Behavior frame continues to exist and is
  // revealed again when the reflex overlay ends.
  bool direct_active = false;
  if (have_direct_reflex_) {
    const uint32_t age = now_ms - direct_reflex_.created_ms;
    switch (direct_reflex_.type) {
      case reflex::ReflexIntentType::TOUCH_RESPONSE:
        direct_active = age < kTouchResponseMs;
        break;
      case reflex::ReflexIntentType::WAKE_WORD_RESPONSE:
        direct_active = age < kWakeWordResponseMs;
        break;
      case reflex::ReflexIntentType::STARTLE:
        direct_active = age < kLiftStartResponseMs;
        break;
      case reflex::ReflexIntentType::SHAKE_RESPONSE:
        direct_active = age < kShakeResponseMs;
        break;
      default:
        direct_active = false;
        break;
    }
    if (direct_active) {
      // Safety/direct physical reflexes hide decorative output. Wake-word is
      // interpersonal, so keep Behavior's NOTICE cue visible while the body
      // acknowledges the caller.
      body.face.prop = face::VisualProp::NONE;
      body.face.prop_progress = 0.0f;
      if (direct_reflex_.type != reflex::ReflexIntentType::WAKE_WORD_RESPONSE) {
        body.face.effect = face::VisualEffect::NONE;
        body.face.effect_progress = 0.0f;
      }
      applyDirectReflex(body, direct_reflex_, now_ms);
    }
  }

  if (!direct_active && have_visual_reflex_) {
    const uint32_t age = now_ms - visual_reflex_.created_ms;
    if (age < kVisualResponseMs) {
      applyVisualReflex(body, visual_reflex_, now_ms);
    }
  }

  return body;
}

void BodyOutputComposer::applyDirectReflex(
    BodyFrame& body,
    const reflex::ReflexIntent& intent,
    uint32_t now_ms) const {
  auto& expression = body.face;
  const uint32_t age = now_ms - intent.created_ms;

  if (intent.type == reflex::ReflexIntentType::TOUCH_RESPONSE) {
    const float amount =
        1.0f - static_cast<float>(age) /
                   static_cast<float>(kTouchResponseMs);
    const float a = clamp01(amount);

    expression.left.lower_lid =
        larger(expression.left.lower_lid, 0.22f * a);
    expression.left.radius_scale =
        larger(expression.left.radius_scale, 1.0f + 0.20f * a);
    expression.left.tilt = -0.45f * a;
    expression.right = expression.left;
    expression.left.openness = clamp01(expression.left.openness + 0.10f);
    expression.right.openness = clamp01(expression.right.openness + 0.10f);
    expression.offset_x *= 0.35f;
    expression.offset_y = clampSigned(expression.offset_y - 0.08f);
    body.neck_pitch = clampSigned(body.neck_pitch - 0.10f * a);
    return;
  }

  if (intent.type == reflex::ReflexIntentType::WAKE_WORD_RESPONSE) {
    const float p = clamp01(
        static_cast<float>(age) / static_cast<float>(kWakeWordResponseMs));
    const float envelope = sinf(p * 3.14159265f);

    // Deliberately readable but not exaggerated: eyes open, head lifts,
    // then a tiny "ん?" cant. This overlays low-level Vision for under a second.
    expression.left.openness =
        clamp01(expression.left.openness + 0.20f * envelope);
    expression.right.openness =
        clamp01(expression.right.openness + 0.20f * envelope);
    expression.left.height_scale =
        larger(expression.left.height_scale, 1.0f + 0.10f * envelope);
    expression.right.height_scale =
        larger(expression.right.height_scale, 1.0f + 0.10f * envelope);
    expression.left.width_scale =
        larger(expression.left.width_scale, 1.0f + 0.06f * envelope);
    expression.right.width_scale =
        larger(expression.right.width_scale, 1.0f + 0.06f * envelope);

    // Keep the NOTICE symbol from Behavior visible during the direct response.
    // Unlike strong safety reflexes, this interpersonal reflex does not hide it.
    body.neck_pitch =
        clampSigned(body.neck_pitch + 0.34f * envelope);
    body.neck_yaw =
        clampSigned(body.neck_yaw +
                    sinf(p * 6.28318531f) * 0.10f * envelope);
    expression.offset_y =
        clampSigned(expression.offset_y - 0.07f * envelope);
    return;
  }

  if (intent.type == reflex::ReflexIntentType::STARTLE) {
    const float amount =
        1.0f - static_cast<float>(age) /
                   static_cast<float>(kLiftStartResponseMs);
    const float a = clamp01(amount);

    expression.left.height_scale =
        larger(expression.left.height_scale, 1.0f + 0.18f * a);
    expression.left.width_scale =
        larger(expression.left.width_scale, 1.0f + 0.04f * a);
    expression.right.height_scale =
        larger(expression.right.height_scale, 1.0f + 0.18f * a);
    expression.right.width_scale =
        larger(expression.right.width_scale, 1.0f + 0.04f * a);
    expression.spacing_scale =
        larger(expression.spacing_scale, 1.0f + 0.025f * a);
    expression.left.openness = clamp01(expression.left.openness + 0.18f);
    expression.right.openness = clamp01(expression.right.openness + 0.18f);
    expression.offset_x *= 0.20f;
    expression.offset_y = clampSigned(expression.offset_y + 0.10f);

    if (age < kLiftStartMouthMs) {
      expression.mouth_open = larger(
          expression.mouth_open,
          1.0f - static_cast<float>(age) /
                     static_cast<float>(kLiftStartMouthMs));
    }
    body.neck_pitch = clampSigned(body.neck_pitch - 0.28f * a);
    return;
  }

  if (intent.type == reflex::ReflexIntentType::SHAKE_RESPONSE) {
    const float amount =
        1.0f - static_cast<float>(age) /
                   static_cast<float>(kShakeResponseMs);
    const float a = clamp01(amount);
    const float phase = static_cast<float>(age) * 0.035f;

    expression.left.openness = clamp01(expression.left.openness + 0.12f);
    expression.right.openness = clamp01(expression.right.openness + 0.12f);
    expression.jitter_x += sinf(phase) * 0.65f * a;
    expression.jitter_y += sinf(phase * 0.7f) * 0.30f * a;
    expression.left.upper_lid =
        larger(expression.left.upper_lid, 0.18f * a);
    expression.right.height_scale =
        larger(expression.right.height_scale, 1.0f + 0.12f * a);

    const float shake_bias = sinf(static_cast<float>(age) * 0.045f) * 0.05f;
    expression.left.openness = clamp01(expression.left.openness + shake_bias);
    expression.right.openness = clamp01(expression.right.openness - shake_bias);
    body.neck_yaw =
        clampSigned(body.neck_yaw + sinf(phase) * 0.35f * a);
    body.neck_pitch =
        clampSigned(body.neck_pitch + sinf(phase * 0.7f) * 0.12f * a);
  }
}

void BodyOutputComposer::applyVisualReflex(
    BodyFrame& body,
    const reflex::ReflexIntent& intent,
    uint32_t now_ms) const {
  auto& expression = body.face;
  const uint32_t age = now_ms - intent.created_ms;

  if (intent.cause == nerve::NeuronType::MOTION_DETECTED) {
    float amount = 0.0f;
    if (age < kMotionAttackMs) {
      amount = static_cast<float>(age) /
               static_cast<float>(kMotionAttackMs);
    } else if (age < kMotionAttackMs + kMotionHoldMs) {
      amount = 1.0f;
    } else {
      const uint32_t release_age =
          age - kMotionAttackMs - kMotionHoldMs;
      amount =
          1.0f - static_cast<float>(release_age) /
                     static_cast<float>(kMotionReleaseMs);
    }
    amount = clamp01(amount);

    expression.left.openness =
        clamp01(expression.left.openness + 0.10f * amount);
    expression.right.openness =
        clamp01(expression.right.openness + 0.10f * amount);

    // Pupil-less face: direction is shown by eye silhouette, not slow travel.
    expression.offset_x = 0.0f;
    expression.offset_y = 0.0f;

    const float x =
        clampSigned(static_cast<float>(intent.payload.x) / 1000.0f);
    const float y =
        clampSigned(static_cast<float>(intent.payload.y) / 1000.0f);
    body.neck_yaw = clampSigned(-x * 0.55f * amount);
    body.neck_pitch = clampSigned(y * 0.28f * amount);
    const float abs_x = x < 0.0f ? -x : x;
    if (abs_x >= kMotionDirectionDeadzone) {
      const float emphasis = amount;

      // Camera/image X is opposite the screen side for a robot facing the user.
      if (x > 0.0f) {
        expression.left.height_scale = 1.0f + 0.25f * emphasis;
        expression.left.width_scale = 1.0f - 0.20f * emphasis;
        expression.right.height_scale = 1.0f - 0.40f * emphasis;
        expression.right.width_scale = 1.0f + 0.20f * emphasis;
      } else {
        expression.right.height_scale = 1.0f + 0.25f * emphasis;
        expression.right.width_scale = 1.0f - 0.20f * emphasis;
        expression.left.height_scale = 1.0f - 0.40f * emphasis;
        expression.left.width_scale = 1.0f + 0.20f * emphasis;
      }
    }
    return;
  }

  const float amount =
      clamp01(1.0f - static_cast<float>(age) /
                         static_cast<float>(kVisualResponseMs));

  if (intent.cause == nerve::NeuronType::BRIGHTER) {
    expression.left.openness =
        clamp01(expression.left.openness - 0.10f * amount);
    expression.right.openness =
        clamp01(expression.right.openness - 0.10f * amount);
  } else if (intent.cause == nerve::NeuronType::DARKER) {
    expression.left.openness =
        clamp01(expression.left.openness + 0.06f * amount);
    expression.right.openness =
        clamp01(expression.right.openness + 0.06f * amount);
  }
}

BodyOutputComposer::ReflexPriority BodyOutputComposer::priorityOf(
    const reflex::ReflexIntent& intent) {
  switch (intent.type) {
    case reflex::ReflexIntentType::STARTLE:
    case reflex::ReflexIntentType::SHAKE_RESPONSE:
      return ReflexPriority::STRONG;

    case reflex::ReflexIntentType::TOUCH_RESPONSE:
    case reflex::ReflexIntentType::WAKE_WORD_RESPONSE:
      return ReflexPriority::INTERPERSONAL;

    case reflex::ReflexIntentType::LOOK_TOWARD_SOURCE:
    case reflex::ReflexIntentType::LIGHT_ADAPT:
      return ReflexPriority::VISUAL;

    default:
      return ReflexPriority::NONE;
  }
}

bool BodyOutputComposer::directIntentActiveAt(
    const reflex::ReflexIntent& intent,
    uint32_t now_ms) {
  const uint32_t age = now_ms - intent.created_ms;
  switch (intent.type) {
    case reflex::ReflexIntentType::TOUCH_RESPONSE:
      return age < kTouchResponseMs;
    case reflex::ReflexIntentType::WAKE_WORD_RESPONSE:
      return age < kWakeWordResponseMs;
    case reflex::ReflexIntentType::STARTLE:
      return age < kLiftStartResponseMs;
    case reflex::ReflexIntentType::SHAKE_RESPONSE:
      return age < kShakeResponseMs;
    default:
      return false;
  }
}

bool BodyOutputComposer::isVisualCause(nerve::NeuronType type) {
  return type == nerve::NeuronType::MOTION_DETECTED ||
         type == nerve::NeuronType::BRIGHTER ||
         type == nerve::NeuronType::DARKER;
}

float BodyOutputComposer::clamp01(float value) {
  if (value < 0.0f) return 0.0f;
  if (value > 1.0f) return 1.0f;
  return value;
}

float BodyOutputComposer::clampSigned(float value) {
  if (value < -1.0f) return -1.0f;
  if (value > 1.0f) return 1.0f;
  return value;
}

float BodyOutputComposer::larger(float a, float b) {
  return a > b ? a : b;
}

}  // namespace body
}  // namespace deskbot

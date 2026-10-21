#pragma once
#include <Arduino.h>
#include "../nerve/NerveTypes.h"

namespace deskbot { namespace ghost {
// Value snapshot assembled BEFORE the current experience is staged. No event
// ownership, trust score, intent inference or Memory dependency in consumers.
// Heart and Behavior share this same pre-event evidence.
struct HeartEventEvidence {
  nerve::PersonId person_id = nerve::kUnknownPerson;
  nerve::NeuronType type = nerve::NeuronType::NONE;
  bool has_prior = false;
  float prior_mood_delta = 0.0f;
  float prior_affection_delta = 0.0f;
  size_t relationship_experiences = 0;
  size_t positive_experiences = 0;
  size_t negative_experiences = 0;
};
struct HeartAppraisal {
  bool used_prior = false;
  bool used_relationship = false;
  float mood_scale = 1.0f;
  float affection_scale = 1.0f;
};
} }  // namespace deskbot::ghost

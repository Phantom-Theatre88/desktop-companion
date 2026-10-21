#pragma once

#include <cmath>
#include "MemoryEngine.h"

namespace deskbot {
namespace ghost {

// A read-only evaluation of currently available experience, not a permanent
// trust score. Positive and negative evidence coexist; no intent is inferred.
struct RelationshipContext {
  nerve::PersonId person_id = nerve::kUnknownPerson;
  size_t experience_count = 0;
  size_t positive_experience_count = 0;
  size_t negative_experience_count = 0;
  bool has_evidence = false;
};

class RelationshipEngine {
 public:
  void begin(uint32_t now_ms) { (void)now_ms; memory_ = nullptr; }
  void bindMemory(const MemoryEngine& memory) { memory_ = &memory; }

  RelationshipContext evaluate(nerve::PersonId person_id) const {
    RelationshipContext result;
    result.person_id = person_id;
    if (memory_ != nullptr)
      memory_->visitPersonExperiences(person_id, evaluateExperience, &result);
    result.has_evidence = result.experience_count != 0;
    return result;
  }

  void tick(uint32_t now_ms) { (void)now_ms; }

 private:
  static void evaluateExperience(const MemoryRecord& record, void* context) {
    auto& result = *static_cast<RelationshipContext*>(context);
    ++result.experience_count;
    // Use recorded reception, never event-name stereotypes or absolute mood.
    // These are descriptive signs, not learned affection/trust or Comfort.
    const float mood = record.outcome_context.state.mood -
                       record.heart_context.state.mood;
    const float affection = record.outcome_context.state.affection -
                            record.heart_context.state.affection;
    if (std::isfinite(mood) && std::isfinite(affection)) {
      if (mood > 0.0f || affection > 0.0f)
        ++result.positive_experience_count;
      if (mood < 0.0f || affection < 0.0f)
        ++result.negative_experience_count;
    }
  }

  // Memory alone owns experience. No event log or independent person cache.
  const MemoryEngine* memory_ = nullptr;
};

}  // namespace ghost
}  // namespace deskbot

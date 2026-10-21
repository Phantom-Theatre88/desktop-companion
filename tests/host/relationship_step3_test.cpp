#include <cassert>
#include <vector>
#include <iostream>
#include "../../firmware/cores3_base/src/vision/CameraVisionInput.h"
#include "../../firmware/cores3_base/src/core/DesktopCompanionRuntime.h"
// Persistence is not under test; use an empty first-boot store on the host.
namespace deskbot { namespace ghost {
bool HeartPersistence::hasPrimarySnapshot() { return false; }
bool HeartPersistence::loadPrimary(HeartState&) { return false; }
bool HeartPersistence::savePrimary(const HeartState&) { return true; }
void HeartPersistence::setMicroSdBackupHandlers(HeartBackupSaveHandler, HeartBackupLoadHandler, void*) {}
bool HeartPersistence::microSdBackupAvailable() const { return false; }
bool HeartPersistence::loadBackup(HeartState&) const { return false; }
bool HeartPersistence::saveBackup(const HeartState&) const { return false; }
}}

using namespace deskbot;
int main() {
  ghost::MemoryEngine memory;
  ghost::RelationshipEngine relationship;
  relationship.begin(0);
  assert(!relationship.evaluate(1).has_evidence);
  memory.begin(0);
  relationship.bindMemory(memory);
  auto event = nerve::makeNeuron(nerve::NeuronType::TOUCH,
                                 nerve::NeuronSource::TOUCH, 100);
  assert(event.person_id == nerve::kUnknownPerson);
  ghost::HeartContext before, after;
  after.state.mood += .03f;
  auto commit = [&](nerve::PersonId person) {
    event.person_id = person;
    memory.beginNeuronExperience(event, before);
    assert(memory.commitNeuronExperience(after));
  };
  commit(0);
  assert(!relationship.evaluate(0).has_evidence);
  event.person_id = 1;
  memory.beginNeuronExperience(event, before);
  assert(!relationship.evaluate(1).has_evidence);
  assert(memory.commitNeuronExperience(after));
  assert(!memory.commitNeuronExperience(after));
  auto a = relationship.evaluate(1);
  assert(a.experience_count == 1 && a.positive_experience_count == 1);
  assert(a.negative_experience_count == 0);
  assert(!relationship.evaluate(2).has_evidence);
  after.state.mood = before.state.mood - .04f;
  commit(2);
  assert(relationship.evaluate(2).negative_experience_count == 1);
  assert(relationship.evaluate(1).positive_experience_count == 1);
  // Contradictory reception is retained in both directions, not cancelled.
  after.state.affection += .01f;
  commit(1);
  a = relationship.evaluate(1);
  assert(a.experience_count == 2 && a.positive_experience_count == 2);
  assert(a.negative_experience_count == 1);
  ghost::MemoryRecord original;
  assert(memory.recallLatest(nerve::NeuronType::TOUCH, original));
  relationship.evaluate(1);
  ghost::MemoryRecord unchanged;
  memory.recallLatest(nerve::NeuronType::TOUCH, unchanged);
  assert(unchanged.heart_context.state.mood == original.heart_context.state.mood);
  assert(unchanged.outcome_context.state.mood == original.outcome_context.state.mood);
  // Evicted evidence cannot survive in a separate Relationship cache.
  for (int i=0; i<8; ++i) commit(2);
  assert(!relationship.evaluate(1).has_evidence);
  assert(relationship.evaluate(2).experience_count == 8);
  memory.begin(0);
  assert(!relationship.evaluate(2).has_evidence);

  ghost::GhostCore ghost;
  ghost.begin(0);
  ghost.tick(100);
  event.person_id = 7;
  ghost.onNeuron(event);
  assert(ghost.relationshipEngine().evaluate(7).experience_count == 1);
  assert(!ghost.relationshipEngine().evaluate(8).has_evidence);
  event.person_id = 0;
  ghost.onNeuron(event);
  assert(ghost.relationshipEngine().evaluate(7).experience_count == 1);
  ghost.begin(200);
  assert(!ghost.relationshipEngine().evaluate(7).has_evidence);
  std::cout << "PASS: Relationship Step 3 person isolation, pending exclusion, mixed evidence, eviction, immutable Memory, Ghost integration/reset\n";
}

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


#include <cmath>
#include <limits>
using namespace deskbot;
static bool near(float a, float b) { return std::fabs(a-b)<1e-6f; }
static void seed(ghost::GhostCore& g, nerve::PersonId person, float mood,
                 float affection, nerve::NeuronType type=nerve::NeuronType::TOUCH) {
  ghost::HeartContext before, after;
  after.state.mood += mood;
  after.state.affection += affection;
  auto n=nerve::makeNeuron(type, nerve::NeuronSource::TOUCH, 10);
  n.person_id=person;
  g.memoryEngine().beginNeuronExperience(n,before);
  assert(g.memoryEngine().commitNeuronExperience(after));
}
int main() {
  auto touch=nerve::makeNeuron(nerve::NeuronType::TOUCH,
                              nerve::NeuronSource::TOUCH, 100);
  touch.person_id=1;
  ghost::GhostCore fresh, positive, negative, other, anonymous;
  for(auto* g : {&fresh,&positive,&negative,&other,&anonymous}) {
    g->begin(0); g->tick(100);
  }
  seed(positive,1,.03f,.02f);
  seed(negative,1,-.06f,-.02f);
  seed(other,2,-.06f,-.02f);
  seed(anonymous,0,-.06f,-.02f);
  ghost::MemoryRecord original;
  positive.memoryEngine().recallLatestForPerson(touch.type,1,original);
  for(auto* g : {&fresh,&positive,&negative,&other,&anonymous}) g->onNeuron(touch);
  assert(!fresh.heartEngine().lastAppraisal().used_prior);
  assert(!fresh.heartEngine().lastAppraisal().used_relationship);
  assert(positive.heartEngine().lastAppraisal().used_prior);
  assert(positive.heartEngine().lastAppraisal().used_relationship);
  assert(positive.heart().mood > fresh.heart().mood);
  assert(positive.heart().affection > fresh.heart().affection);
  assert(negative.heart().mood < fresh.heart().mood);
  assert(near(other.heart().mood,fresh.heart().mood));
  assert(near(anonymous.heart().mood,fresh.heart().mood));
  const auto& committed=positive.memoryEngine().lastCandidate();
  assert(near(committed.heart_context.state.mood,.60f));
  assert(committed.heart_context.captured_ms==100);
  assert(near(committed.outcome_context.state.mood,positive.heart().mood));
  assert(near(original.heart_context.state.mood,.60f));
  assert(near(original.outcome_context.state.mood,.63f));
  assert(positive.memoryEngine().workingExperienceCount()==2);
  assert(committed.appraisal_evidence.relationship_experiences==1);
  assert(committed.appraisal_evidence.has_prior);
  assert(near(committed.appraisal_evidence.prior_mood_delta,.03f));
  // Verify the original record inside Memory, not just a previously copied value.
  bool foundOriginal=false;
  positive.memoryEngine().visitPersonExperiences(1,
      [](const ghost::MemoryRecord& r,void* context) {
        if(r.occurred_ms==10) {
          assert(near(r.heart_context.state.mood,.60f));
          assert(near(r.outcome_context.state.mood,.63f));
          *static_cast<bool*>(context)=true;
        }
      }, &foundOriginal);
  assert(foundOriginal);

  // Different type for the same person contributes Relationship, not exact recall.
  ghost::GhostCore relationOnly;
  relationOnly.begin(0); seed(relationOnly,1,-.06f,0,nerve::NeuronType::SHAKE);
  relationOnly.tick(100); relationOnly.onNeuron(touch);
  assert(!relationOnly.heartEngine().lastAppraisal().used_prior);
  assert(relationOnly.heartEngine().lastAppraisal().used_relationship);
  assert(relationOnly.heart().mood < fresh.heart().mood);

  // Unknown people are never treated as the same interpersonal partner.
  ghost::GhostCore unknown;
  unknown.begin(0); seed(unknown,0,.03f,.02f);
  touch.person_id=0; unknown.tick(100); unknown.onNeuron(touch);
  assert(!unknown.heartEngine().lastAppraisal().used_prior);
  assert(!unknown.heartEngine().lastAppraisal().used_relationship);
  touch.person_id=1;

  ghost::HeartEngine neutral, happy, unhappy;
  for(auto* h : {&neutral,&happy,&unhappy}) h->begin(0);
  auto context=neutral.snapshot(100), high=context, low=context;
  high.state.mood=.9f; low.state.mood=.2f;
  neutral.onNeuron(touch,context); happy.onNeuron(touch,high);
  unhappy.onNeuron(touch,low);
  assert(happy.state().mood > neutral.state().mood);
  assert(unhappy.state().mood < neutral.state().mood);
  assert(near(happy.state().affection,neutral.state().affection));

  // Malformed/mismatched evidence is ignored; numeric state stays finite.
  ghost::HeartEventEvidence evidence;
  evidence.type=touch.type; evidence.person_id=1; evidence.has_prior=true;
  evidence.prior_mood_delta=std::numeric_limits<float>::quiet_NaN();
  evidence.relationship_experiences=1; evidence.positive_experiences=2;
  ghost::HeartEngine invalid; invalid.begin(0); invalid.onNeuron(touch,context,evidence);
  assert(!invalid.lastAppraisal().used_prior && !invalid.lastAppraisal().used_relationship);
  assert(near(invalid.state().mood,neutral.state().mood));
  evidence.prior_mood_delta=.1f; evidence.person_id=2;
  invalid.begin(0); invalid.onNeuron(touch,context,evidence);
  assert(!invalid.lastAppraisal().used_prior);

  // Mixed evidence can strengthen affection while tempering mood.
  evidence.person_id=1; evidence.has_prior=false;
  evidence.positive_experiences=1; evidence.negative_experiences=1;
  invalid.begin(0); invalid.onNeuron(touch,context,evidence);
  assert(invalid.state().mood < neutral.state().mood);
  assert(invalid.state().affection > neutral.state().affection);

  // Appraisal cannot attenuate SHAKE, or alter Reflex intent/sensitivity.
  auto shake=nerve::makeNeuron(nerve::NeuronType::SHAKE,nerve::NeuronSource::IMU,100);
  ghost::HeartEngine safeA,safeB; safeA.begin(0);safeB.begin(0);
  evidence.type=shake.type; evidence.person_id=0;
  safeA.onNeuron(shake,context);safeB.onNeuron(shake,context,evidence);
  assert(near(safeA.state().mood,safeB.state().mood));
  assert(near(safeA.state().attention,safeB.state().attention));
  assert(!safeB.lastAppraisal().used_relationship);
  reflex::ReflexLayer reflex;
  reflex::ReflexIntent intent;
  reflex.setIntentHandler([](const reflex::ReflexIntent& i,void* ctx) {
    *static_cast<reflex::ReflexIntent*>(ctx)=i;
  }, &intent);
  reflex.onNeuron(shake);
  assert(intent.type==reflex::ReflexIntentType::SHAKE_RESPONSE);

  // Long repetition is bounded, and fixed baselines do not silently learn.
  for(int i=0;i<1000;++i) {
    touch.timestamp_ms=200+i;
    positive.tick(touch.timestamp_ms); positive.onNeuron(touch);
    const auto& a=positive.heartEngine().lastAppraisal();
    assert(a.mood_scale>=.75f && a.mood_scale<=1.25f);
    assert(a.affection_scale>=.75f && a.affection_scale<=1.25f);
    assert(std::isfinite(positive.heart().mood));
    assert(positive.heart().mood<=1 && positive.heart().affection<=1);
  }
  assert(near(positive.heartEngine().baseline().mood,.60f));
  positive.begin(2000);
  assert(!positive.heartEngine().lastAppraisal().used_prior);
  std::cout << "PASS: Heart Step 4 prior/relationship/state appraisal, identity isolation, pre-event ordering, snapshots, malformed/mixed evidence, safety, bounds, reset\n";
}

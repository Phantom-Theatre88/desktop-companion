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

#include "../../firmware/cores3_base/src/body/BodyOutputComposer.h"
std::vector<DrawCall> M5Canvas::rects;
int M5Canvas::pushes=0;
using Response = ghost::InterpersonalResponse;
int main() {
  auto call=nerve::makeNeuron(nerve::NeuronType::WAKE_WORD_DETECTED,
                             nerve::NeuronSource::PI5,900);
  call.person_id=1;
  core::DesktopCompanionRuntime fresh, positive, negative, mixed, other, unknown;
  for (auto* runtime : {&fresh,&positive,&negative,&mixed,&other,&unknown}) {
    assert(runtime->begin(0)); runtime->tick(1000);
  }
  seed(positive.ghost(),1,.03f,.02f,call.type);
  seed(negative.ghost(),1,-.03f,-.02f,call.type);
  seed(mixed.ghost(),1,.03f,.02f,call.type);
  seed(mixed.ghost(),1,-.03f,-.02f); // Relationship across different events.
  seed(other.ghost(),2,.03f,.02f,call.type);
  seed(unknown.ghost(),0,.03f,.02f,call.type);
  for (auto* runtime : {&fresh,&positive,&negative,&mixed,&other,&unknown}) {
    runtime->emit(call); runtime->tick(1500);
  }
  auto decision=[](core::DesktopCompanionRuntime& r) {
    return r.ghost().behaviorEngine().interpersonalDecision();
  };
  assert(decision(fresh).response==Response::STANDARD);
  assert(!decision(fresh).used_prior && !decision(fresh).used_relationship);
  assert(decision(positive).response==Response::OPEN);
  assert(decision(negative).response==Response::MEASURED);
  assert(decision(mixed).response==Response::MIXED);
  assert(decision(other).response==Response::STANDARD);
  assert(decision(unknown).response==Response::STANDARD);
  // Identical current Heart states: differences come directly from evidence.
  assert(near(fresh.ghost().heart().attention,positive.ghost().heart().attention));
  const auto standard=fresh.ghost().behaviorEngine().microBehavior();
  const auto open=positive.ghost().behaviorEngine().microBehavior();
  const auto reserved=negative.ghost().behaviorEngine().microBehavior();
  const auto both=mixed.ghost().behaviorEngine().microBehavior();
  assert(open.left_shape.width_scale>standard.left_shape.width_scale);
  assert(reserved.left_shape.width_scale<standard.left_shape.width_scale);
  assert(both.left_shape.width_scale>reserved.left_shape.width_scale);
  assert(both.left_shape.width_scale<standard.left_shape.width_scale);
  assert(open.neck_pitch>standard.neck_pitch);
  const auto& record=positive.ghost().memoryEngine().lastCandidate();
  assert(record.appraisal_evidence.relationship_experiences==1);
  assert(record.appraisal_evidence.has_prior);
  assert(record.heart_context.captured_ms==1000);
  assert(near(record.heart_context.state.attention,.50f));
  assert(near(record.outcome_context.state.attention,positive.ghost().heart().attention));
  assert(positive.ghost().memoryEngine().workingExperienceCount()==2);

  // Isolate prior-only and Relationship-only paths with a fixed Heart frame.
  ghost::BehaviorEngine behavior;
  ghost::HeartContext context; context.captured_ms=1000;
  ghost::HeartEventEvidence evidence;
  evidence.type=call.type; evidence.person_id=1; evidence.has_prior=true;
  evidence.prior_mood_delta=-.03f;
  behavior.begin(0); behavior.onNeuron(call,context,evidence);
  assert(behavior.interpersonalDecision().response==Response::MEASURED);
  assert(behavior.interpersonalDecision().used_prior);
  assert(!behavior.interpersonalDecision().used_relationship);
  evidence.has_prior=false; evidence.relationship_experiences=1;
  evidence.positive_experiences=1;
  behavior.onNeuron(call,context,evidence);
  assert(behavior.interpersonalDecision().response==Response::OPEN);
  assert(!behavior.interpersonalDecision().used_prior);
  assert(behavior.interpersonalDecision().used_relationship);
  evidence.has_prior=true; evidence.prior_mood_delta=-.03f;
  behavior.onNeuron(call,context,evidence);
  assert(behavior.interpersonalDecision().response==Response::MIXED);
  evidence.prior_mood_delta=std::numeric_limits<float>::quiet_NaN();
  evidence.positive_experiences=2;
  behavior.onNeuron(call,context,evidence);
  assert(behavior.interpersonalDecision().response==Response::STANDARD);
  evidence.person_id=2; evidence.prior_mood_delta=.03f;
  evidence.positive_experiences=1;
  behavior.onNeuron(call,context,evidence);
  assert(behavior.interpersonalDecision().response==Response::STANDARD);
  evidence.person_id=1; evidence.type=nerve::NeuronType::TOUCH;
  behavior.onNeuron(call,context,evidence);
  assert(behavior.interpersonalDecision().response==Response::STANDARD);
  evidence.type=call.type;
  call.person_id=0; evidence.person_id=0;
  behavior.onNeuron(call,context,evidence);
  assert(behavior.interpersonalDecision().response==Response::STANDARD);
  call.person_id=1; evidence.person_id=1;

  // Start time zero and clock wrap are both valid handling-time clocks.
  context.captured_ms=0; behavior.begin(0);
  behavior.onNeuron(call,context,evidence); behavior.tick(500,context);
  assert(behavior.interpersonalResponseActive());
  assert(behavior.microBehavior().interpersonal_priority);
  behavior.tick(1100,context);
  assert(!behavior.interpersonalResponseActive());
  assert(behavior.interpersonalDecision().response==Response::STANDARD);
  assert(!behavior.microBehavior().interpersonal_priority);
  context.captured_ms=UINT32_MAX-99;
  behavior.begin(context.captured_ms); behavior.onNeuron(call,context,evidence);
  behavior.tick(400,context);
  assert(behavior.interpersonalResponseActive());
  behavior.tick(1000,context);
  assert(!behavior.interpersonalResponseActive());

  // A second caller without evidence gets standard, not the previous decision.
  context.captured_ms=1000; behavior.begin(0);
  behavior.onNeuron(call,context,evidence);
  call.person_id=2; context.captured_ms=1200;
  behavior.onNeuron(call,context,{});
  assert(behavior.interpersonalDecision().response==Response::STANDARD);
  assert(behavior.interpersonalDecision().person_id==2);
  call.person_id=1;

  // Person recognition absence and physical interference never create a policy.
  auto noise=nerve::makeNeuron(nerve::NeuronType::MOTION_DETECTED,
                               nerve::NeuronSource::CAMERA_M5,1600);
  behavior.begin(0); behavior.onNeuron(noise,context,evidence);
  assert(!behavior.interpersonalResponseActive());
  context.captured_ms=1000; behavior.onNeuron(call,context,evidence);
  behavior.tick(1200,context);
  body::BodyOutputComposer composer; composer.begin(0);
  reflex::ReflexIntent visual;
  visual.type=reflex::ReflexIntentType::LOOK_TOWARD_SOURCE;
  visual.cause=nerve::NeuronType::MOTION_DETECTED;
  visual.created_ms=1100; visual.payload.x=800;
  composer.onReflexIntent(visual);
  auto output=composer.compose(behavior.microBehavior(),1200);
  assert(near(output.face.left.width_scale,behavior.microBehavior().left_shape.width_scale));
  reflex::ReflexIntent shake;
  shake.type=reflex::ReflexIntentType::SHAKE_RESPONSE;
  shake.cause=nerve::NeuronType::SHAKE; shake.created_ms=1200;
  composer.onReflexIntent(shake);
  output=composer.compose(behavior.microBehavior(),1240);
  assert(output.face.jitter_x!=0 && output.face.left.upper_lid>0);
  // The runtime forwards SHAKE to Behavior, cancelling the old acknowledgement.
  auto event=nerve::makeNeuron(nerve::NeuronType::SHAKE,nerve::NeuronSource::IMU,1600);
  positive.tick(1600); positive.emit(event); positive.tick(1640);
  assert(!positive.ghost().behaviorEngine().interpersonalResponseActive());
  assert(positive.ghost().behaviorEngine().interpersonalDecision().response==Response::STANDARD);
  positive.tick(2100);
  assert(!positive.ghost().behaviorEngine().microBehavior().interpersonal_priority);
  // Evicted evidence cannot leak from an earlier event decision.
  for(int i=0;i<8;++i) seed(positive.ghost(),0,0,0);
  positive.tick(2400); positive.emit(call);
  assert(decision(positive).response==Response::STANDARD);
  assert(!decision(positive).used_prior && !decision(positive).used_relationship);

  // Current Heart still controls expression strength for the same selection.
  ghost::BehaviorEngine lowHeart, highHeart;
  ghost::HeartContext low=context, high=context;
  low.captured_ms=high.captured_ms=1000;
  low.state.attention=.2f; high.state.attention=.8f;
  lowHeart.begin(0); highHeart.begin(0);
  lowHeart.onNeuron(call,low,evidence); highHeart.onNeuron(call,high,evidence);
  lowHeart.tick(1500,low); highHeart.tick(1500,high);
  assert(lowHeart.interpersonalDecision().response==highHeart.interpersonalDecision().response);
  assert(highHeart.microBehavior().left_shape.width_scale>
         lowHeart.microBehavior().left_shape.width_scale);

  // Reset clears transient decisions; repeated evaluation is deterministic.
  behavior.begin(0);
  assert(!behavior.interpersonalResponseActive());
  for(int i=0;i<100;++i) {
    behavior.onNeuron(call,context,evidence); behavior.tick(1500,context);
    assert(behavior.interpersonalDecision().response==Response::OPEN);
    assert(std::isfinite(behavior.microBehavior().neck_pitch));
  }
  std::cout << "PASS: Behavior Step 5 evidence-driven selection/output, identical Heart, person isolation, prior-only/relationship-only/mixed, invalid input, pre-event snapshots, expiry/zero/wrap, caller replacement, safety and reset\n";
}

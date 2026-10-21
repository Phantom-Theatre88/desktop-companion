#include "GhostCore.h"

namespace deskbot {
namespace ghost {

void GhostCore::begin(uint32_t now_ms) {
  last_neuron_type_ = nerve::NeuronType::NONE;
  last_neuron_ms_ = 0;
  heart_.begin(now_ms);
  memory_.begin(now_ms);
  time_.begin(now_ms);
  relationship_.begin(now_ms);
  relationship_.bindMemory(memory_);
  behavior_.begin(now_ms);
  offline_status_ = ElapsedStatus::CLOCK_UNKNOWN;
  if (persistence_.load(checkpoint_)) {
    const auto elapsed = time_.elapsedSince(checkpoint_.unix_seconds);
    offline_status_ = elapsed.status;
    heart_.restoreCheckpoint(checkpoint_.heart, checkpoint_.baseline, elapsed,
                             now_ms, checkpoint_.quiet_seconds);
    memory_.restoreSnapshot(checkpoint_.memory);
  }
  last_checkpoint_attempt_ms_ = now_ms;
  observed_heart_change_ms_ = heart_.lastHeartChangeMs();
  checkpoint_dirty_ = true;
}

void GhostCore::onNeuron(const nerve::SemanticNeuron& neuron) {
  checkpoint_dirty_ = true;
  last_neuron_type_ = neuron.type;
  last_neuron_ms_ = neuron.timestamp_ms;
  // LOCK 35: capture Heart Context once at the start of this event and use
  // the same read-only snapshot throughout the event processing unit.
  // SemanticNeuron::timestamp_ms remains the occurrence time. captured_ms is
  // the Ghost handling-time clock supplied by TimeEngine, which Behavior uses
  // for arbitration/lifetimes so blocking device work cannot age an action.
  const HeartContext event_context = heart_.snapshot(time_.lastTickMs());

  // Read prior Memory and Relationship BEFORE staging the current event.
  HeartEventEvidence evidence;
  evidence.person_id = neuron.person_id;
  evidence.type = neuron.type;
  MemoryRecord prior;
  evidence.has_prior = memory_.recallLatestForPerson(
      neuron.type, neuron.person_id, prior);
  if (evidence.has_prior) {
    evidence.prior_mood_delta =
        prior.outcome_context.state.mood - prior.heart_context.state.mood;
    evidence.prior_affection_delta =
        prior.outcome_context.state.affection - prior.heart_context.state.affection;
  }
  const auto relationship = relationship_.evaluate(neuron.person_id);
  evidence.relationship_experiences = relationship.experience_count;
  evidence.positive_experiences = relationship.positive_experience_count;
  evidence.negative_experiences = relationship.negative_experience_count;
  memory_.beginNeuronExperience(neuron, event_context, evidence);
  heart_.onNeuron(neuron, event_context, evidence);
  // Record the post-stimulus Heart state separately, preserving the shared
  // pre-stimulus snapshot required by LOCK 35.
  memory_.commitNeuronExperience(heart_.snapshot(time_.lastTickMs()));
  // Relationship evaluates committed Memory on demand; no duplicate event feed.
  behavior_.onNeuron(neuron, event_context, evidence);
}

void GhostCore::onReflexResult(const reflex::ReflexResult& result) {
  // LOCK 47-49: the completed reflex result returns to Ghost and is offered to
  // Heart and Memory using one fixed Heart Context snapshot for this result.
  const HeartContext result_context = heart_.snapshot(result.completed_ms);
  heart_.onReflexResult(result, result_context);
  memory_.onReflexResult(result, result_context);
}

void GhostCore::tick(uint32_t now_ms) {
  time_.tick(now_ms);
  heart_.tick(now_ms);
  memory_.tick(now_ms);
  relationship_.tick(now_ms);

  // LOCK 52: standalone DeskRobo life must exist before external sensors.
  // Behavior receives the current read-only Heart snapshot every runtime tick
  // and produces continuous micro-behavior for the body/face layer.
  behavior_.tick(now_ms, heart_.snapshot(now_ms));
  if (observed_heart_change_ms_ != heart_.lastHeartChangeMs()) checkpoint_dirty_ = true;
  observed_heart_change_ms_ = heart_.lastHeartChangeMs();
  // Rate-limit retries as well as successful periodic writes; no event-path I/O.
  if (checkpoint_dirty_ && static_cast<uint32_t>(now_ms-last_checkpoint_attempt_ms_) >= 60000) {
    last_checkpoint_attempt_ms_ = now_ms;
    saveCheckpoint();
  }
}

bool GhostCore::saveCheckpoint() {
  last_checkpoint_attempt_ms_ = time_.lastTickMs();
  checkpoint_.heart = heart_.state();
  checkpoint_.baseline = heart_.baseline();
  checkpoint_.memory = memory_.exportSnapshot();
  checkpoint_.unix_seconds = time_.unixSeconds();
  checkpoint_.quiet_seconds = (time_.lastTickMs()-heart_.lastEventMs())/1000;
  const bool ok = persistence_.save(checkpoint_);
  if (ok) checkpoint_dirty_ = false;
  return ok;
}

void GhostCore::synapseHandler(const nerve::SemanticNeuron& neuron, void* context) {
  auto* self = static_cast<GhostCore*>(context);
  if (self != nullptr) {
    self->onNeuron(neuron);
  }
}

}  // namespace ghost
}  // namespace deskbot

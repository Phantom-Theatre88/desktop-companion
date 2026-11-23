"""Provisional integration adapter, NOT a new personality policy/LOCK.

Only explicit test events and verified NERVA results can affect the
existing autonomous candidate. Unattributed events cannot become
relationship evidence. Persistent relationships and learning are not implemented.
"""
from __future__ import annotations
from dataclasses import dataclass
from uuid import uuid4
from experience_journal import ExperienceJournal
from ghost_core import Ghost, HeartSnapshot, SemanticEvent

@dataclass(frozen=True)
class Selection:
    kind: str
    basis: str
    experience_count: int
    relationship_count: int
    last_result: str | None

class BehaviorBridge:
    def __init__(self, ghost: Ghost, journal: ExperienceJournal | None = None):
        self.ghost = ghost
        self.journal = journal
        self.run_id = uuid4().hex
        if journal is not None:
            # Preserve original event and historical Heart, but never claim
            # that a restart restored present feelings or sleep state.
            for experience in journal.read_all():
                self.ghost.memory.commit(experience)
        self.last_result = None
        self.results: dict[int, dict] = {}
        self.pending: set[int] = set()
        self.intent_kinds: dict[int, str] = {}
        self.failed_intent: str | None = None

    def register_intent(self, sequence: int, kind: str) -> None:
        self.pending.add(sequence)
        self.intent_kinds[sequence] = kind

    def accept_result(self, data: dict) -> bool:
        """Accept only results for outstanding sequences; never infer physical success."""
        if data.get('type') != 'nerva_result' or data.get('version') != 1:
            return False
        seq = data.get('sequence')
        state = data.get('status')
        if type(seq) is not int or seq not in self.pending or state not in ('rejected','applied_frame','completed','interrupted'):
            return False
        if seq in self.results:
            return False
        # Diagnostic result is recorded as an experience, not as a claim of felt emotion.
        t = self.ghost.time.last_tick_ms + 1
        event = SemanticEvent('nerva-result-'+self.run_id+'-'+str(seq), 'nerva_'+state, t)
        snapshot = self.ghost.heart.snapshot()
        # Persist before acknowledging. A disk failure must not be presented as
        # successful durable recording. The in-process result is still diagnostic.
        from ghost_core import Experience
        experience = Experience(event, snapshot, snapshot)
        if self.journal is not None:
            self.journal.append(experience)
        self.ghost.memory.commit(experience)
        self.ghost.time.tick(t)
        self.pending.discard(seq)
        self.results[seq] = {'status': state, 'model_bound': data.get('model_bound') is True}
        self.last_result = state
        if state == 'rejected' or (state == 'applied_frame' and data.get('model_bound') is not True):
            self.failed_intent = self.intent_kinds.get(seq)
        elif state == 'applied_frame' and data.get('model_bound') is True:
            self.failed_intent = None
        return True

    def select(self, candidate: str, actor_id: str | None = None) -> Selection:
        """Read-only evidence flow. Candidate from existing CoreS3 time policy.
        No unapproved relationship-based behavior change: evidence is exposed,
        not converted into an invented affection/trust score.
        """
        memory = self.ghost.memory.recall()
        evidence = self.ghost.relationship.evidence_for(actor_id)
        # Safety fallback only: never retry an action immediately after NERVA
        # reports it cannot bind. This is not learned personality or Relationship.
        blocked = candidate == self.failed_intent and candidate != 'idle'
        return Selection('idle' if blocked else candidate,
                         'nerva_rejection_safety_fallback' if blocked else 'existing_coreS3_candidate', len(memory),
                         len(evidence.events) if evidence else 0, self.last_result)
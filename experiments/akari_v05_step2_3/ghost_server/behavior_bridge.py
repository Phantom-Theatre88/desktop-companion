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
        self.progress: dict[int, dict] = {}  # first bound frame is NOT terminal completion
        self.pending: set[int] = set()
        self.intent_kinds: dict[int, str] = {}
        self.failed_intent: str | None = None

    def register_intent(self, sequence: int, kind: str) -> None:
        self.pending.add(sequence)
        self.intent_kinds[sequence] = kind

    def accept_result(self, data: dict) -> bool:
        """Keep pending through applied_frame; only terminal reports close an action.

        Every accepted event is durable before any acknowledgement. Repeated
        frames, terminal duplicates and contradictory terminal status are ignored.
        """
        if data.get('type') != 'nerva_result' or data.get('version') != 1:
            return False
        seq = data.get('sequence')
        state = data.get('status')
        if type(seq) is not int or seq not in self.pending or state not in ('rejected','applied_frame','completed','interrupted'):
            return False
        if type(data.get('model_bound')) is not bool:
            return False
        bound = data['model_bound']
        if state == 'completed' and not bound:
            return False  # no success claim without a bound avatar
        if state == 'applied_frame' and seq in self.progress:
            return False
        if state != 'applied_frame' and seq in self.results:
            return False
        if state == 'applied_frame' and not bound:
            # No bound model cannot count as a rendered frame.
            return False
        t = self.ghost.time.last_tick_ms + 1
        from ghost_core import Experience
        event = SemanticEvent('nerva-result-'+self.run_id+'-'+str(seq)+'-'+state,
                              'nerva_'+state, t)
        snapshot = self.ghost.heart.snapshot()
        experience = Experience(event, snapshot, snapshot,
                                interpretation='motor intent: '+self.intent_kinds.get(seq, 'unknown'))
        if self.journal is not None:
            self.journal.append(experience)
        self.ghost.memory.commit(experience)
        self.ghost.time.tick(t)
        report = {'status': state, 'model_bound': bound}
        self.last_result = state
        if state == 'applied_frame':
            self.progress[seq] = report
            self.failed_intent = None
        else:
            self.pending.discard(seq)
            self.results[seq] = report
            if state in ('rejected', 'interrupted'):
                self.failed_intent = self.intent_kinds.get(seq)
            elif state == 'completed':
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
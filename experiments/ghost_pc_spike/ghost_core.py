"""PC Ghost boundary spike. NOT a personality/learning algorithm.

Checks that Memory owns factual experiences, Relationship reads them without
rewriting facts, and the event-entry Heart snapshot remains immutable.
Python 3 stdlib only; transport/storage/learned coefficients remain undecided.
"""
from __future__ import annotations

from dataclasses import dataclass, replace
from typing import Optional


@dataclass(frozen=True)
class HeartSnapshot:
    mood: float
    comfort: float
    captured_ms: int


@dataclass(frozen=True)
class SemanticEvent:
    event_id: str
    kind: str
    occurred_ms: int
    actor_id: Optional[str] = None  # Optional proposal; NOT present in old Neuron


@dataclass(frozen=True)
class Experience:
    event: SemanticEvent
    heart_before: HeartSnapshot
    heart_after: HeartSnapshot
    interpretation: Optional[str] = None


class Memory:
    def __init__(self) -> None:
        self._experiences: list[Experience] = []
        self._ids: set[str] = set()

    def commit(self, experience: Experience) -> None:
        if not experience.event.event_id:
            raise ValueError("event_id required")
        if experience.event.event_id in self._ids:
            raise ValueError("duplicate event_id")
        self._experiences.append(experience)
        self._ids.add(experience.event.event_id)

    def recall(self, *, actor_id: Optional[str] = None, kind: Optional[str] = None) -> tuple[Experience, ...]:
        return tuple(x for x in self._experiences
                     if (actor_id is None or x.event.actor_id == actor_id)
                     and (kind is None or x.event.kind == kind))

    def reinterpret(self, event_id: str, interpretation: str) -> None:
        """Replace the current *interpretation*, never event or prior feelings.

        This spike does not yet implement interpretation history; that is an
        explicit gap against the Ghost sacred design.
        """
        for i, x in enumerate(self._experiences):
            if x.event.event_id == event_id:
                self._experiences[i] = replace(x, interpretation=interpretation)
                return
        raise KeyError(event_id)


@dataclass(frozen=True)
class ExperienceContext:
    """Read-only evidence available BEFORE the current event is committed."""
    event: SemanticEvent
    heart_before: HeartSnapshot
    prior_same_kind: tuple[Experience, ...]
    relationship_evidence: Optional["RelationshipEvidence"]


@dataclass(frozen=True)
class ProcessedExperience:
    context: ExperienceContext
    experience: Experience


@dataclass(frozen=True)
class RelationshipEvidence:
    actor_id: str
    events: tuple[Experience, ...]


class Relationship:
    """Read-only evidence view, not an invented trust-score algorithm."""
    def __init__(self, memory: Memory) -> None:
        self._memory = memory

    def evidence_for(self, actor_id: Optional[str]) -> Optional[RelationshipEvidence]:
        if not actor_id:
            return None  # No actor: NEVER ascribe to a particular person.
        events = self._memory.recall(actor_id=actor_id)
        return RelationshipEvidence(actor_id, events)


class Time:
    """Monotonic event clock only; power-off elapsed time unresolved."""
    def __init__(self) -> None:
        self.last_tick_ms = 0

    def tick(self, elapsed_ms: int) -> None:
        if elapsed_ms < self.last_tick_ms:
            raise ValueError("nonmonotonic clock")
        self.last_tick_ms = elapsed_ms


class Heart:
    """Owns the current value; no unapproved learning/decay coefficients."""
    def __init__(self, initial: HeartSnapshot) -> None:
        self._state = initial

    def snapshot(self) -> HeartSnapshot:
        return self._state

    def observe(self, state: HeartSnapshot) -> None:
        self._state = state


class Behavior:
    """Explicit interface boundary; actual policy not yet selected."""
    def describe_inputs(self, heart: HeartSnapshot, evidence: Optional[RelationshipEvidence]) -> dict:
        return {"heart": heart, "evidence_count": len(evidence.events) if evidence else 0}


class Ghost:
    def __init__(self, initial: HeartSnapshot) -> None:
        self.heart = Heart(initial)
        self.memory = Memory()
        self.time = Time()
        self.relationship = Relationship(self.memory)
        self.behavior = Behavior()

    def experience_context(self, event: SemanticEvent) -> ExperienceContext:
        """Capture the past before recording this event.

        No current event may appear among prior evidence. Actor-free events
        must not be attributed to the user or any other person.
        """
        evidence = self.relationship.evidence_for(event.actor_id)
        prior = tuple(x for x in self.memory.recall(kind=event.kind)
                      if x.event.occurred_ms <= event.occurred_ms
                      and (event.actor_id is None or x.event.actor_id == event.actor_id))
        return ExperienceContext(event, self.heart.snapshot(), prior, evidence)

    def remember_then_record(self, event: SemanticEvent,
                             after: HeartSnapshot) -> ProcessedExperience:
        """Learning A Step 2: look up existing experiences, then commit new one.

        This is a data-flow foundation, NOT a learned reaction algorithm.
        """
        self.time.tick(event.occurred_ms)
        context = self.experience_context(event)
        experience = self.record_event(event, after)
        return ProcessedExperience(context, experience)

    def record_event(self, event: SemanticEvent, after: HeartSnapshot) -> Experience:
        """Capture before, commit after. Relationship never writes Memory."""
        before = self.heart.snapshot()
        exp = Experience(event=event, heart_before=before, heart_after=after)
        self.memory.commit(exp)
        self.heart.observe(after)
        return exp

"""Experimental *observed-stage* Ghost Behavior loop.

Not an AKARI personality formula or WORLD perception system.  Uses only
confirmed NERVA feedback, current NERVA body position/posture, existing
Ghost boredom threshold and event Memory.  Ghost picks *meaningful actions*;
NERVA owns how the limbs move.  Every result can change the next decision.
"""
from __future__ import annotations
from dataclasses import dataclass
import math


@dataclass
class StageObservation:
    x: float
    seated: bool
    busy: bool
    bound: bool
    received_ms: int


class LifePolicy:
    """Trial-stage safety policy. Does not create personality/Relationship scores."""

    # Limits are physical trial-stage geometry / action pacing, not emotion weights.
    HOME_X = 0.0
    SEAT_X = -0.65
    POSITION_EPS = 0.13
    REST_MS = 30_000
    ACTIVITY_PAUSE_MS = 20_000

    def __init__(self, memory: tuple = ()):
        self.observation: StageObservation | None = None
        self.pending: str | None = None
        self.previous_completed: str | None = None
        self.completed_at_ms: int = 0
        self.failed: set[str] = set()
        self.decision_basis: str = 'awaiting_current_stage_observation'
        self.completed_count: int = 0
        self.observation_sequence = 0
        self.restore_history(memory)

    def restore_history(self, memory: tuple) -> None:
        # Memory is immutable historical evidence, not a restored live sensor.
        # Recover only the last verified motor outcome and past motor failures.
        for exp in memory:
            kind = exp.interpretation or ''
            if not kind.startswith('motor intent: '):
                continue
            intent = kind[len('motor intent: '):]
            if intent not in ('explore', 'sit', 'rise'):
                continue
            if exp.event.kind in ('nerva_rejected', 'nerva_interrupted'):
                self.failed.add(intent)
            elif exp.event.kind == 'nerva_completed':
                self.previous_completed = intent
                self.failed.discard(intent)

    def observe(self, data: dict, now_ms: int) -> bool:
        if (data.get('type') != 'nerva_observation' or data.get('version') != 1
            or type(data.get('model_bound')) is not bool
            or type(data.get('seated')) is not bool
            or type(data.get('motion_busy')) is not bool):
            return False
        x = data.get('position_x')
        if type(x) not in (int, float) or not math.isfinite(x) or abs(x) > 2:
            return False
        self.observation = StageObservation(float(x), data['seated'],
                                            data['motion_busy'], data['model_bound'], now_ms)
        self.observation_sequence += 1
        return True

    def result(self, kind: str, status: str, now_ms: int, memory: tuple) -> bool:
        """Only terminal outcome after a real correlated NERVA response."""
        if kind != self.pending or status == 'applied_frame':
            return False
        if status == 'completed':
            # Memory must already contain the actual NERVA completion event.
            if not any(x.event.kind == 'nerva_completed'
                       and x.interpretation == 'motor intent: ' + kind for x in memory):
                return False
            self.previous_completed = kind
            self.completed_at_ms = now_ms
            self.completed_count += 1
        else:
            self.failed.add(kind)
        self.pending = None
        # A new fresh body observation is required after completion; don't assume
        # the body reached a stage merely from a clock or returned success bit.
        self.observation = None
        return True

    def choose(self, now_ms: int, boredom: float, sleepiness: float,
               memory: tuple, observation_now_ms: int | None = None) -> str | None:
        o = self.observation
        observed_now = now_ms if observation_now_ms is None else observation_now_ms
        if o is None or not o.bound or observed_now - o.received_ms > 3_000:
            self.decision_basis = 'no_fresh_bound_character_observation'
            return None
        if o.busy or self.pending is not None:
            self.decision_basis = 'waiting_for_actual_nerva_result'
            return None
        at_home = abs(o.x - self.HOME_X) <= self.POSITION_EPS
        at_seat = abs(o.x - self.SEAT_X) <= self.POSITION_EPS
        if not (at_home or at_seat):
            self.decision_basis = 'outside_verified_trial_stage'
            return None
        if self.previous_completed is not None:
            if not any(x.event.kind == 'nerva_completed' and
                       x.interpretation == 'motor intent: ' + self.previous_completed
                       for x in memory):
                self.decision_basis = 'missing_completion_in_memory'
                return None
        if o.seated:
            if not at_seat:
                self.decision_basis = 'unsafe_seated_position'
                return None
            if now_ms - self.completed_at_ms < self.REST_MS:
                self.decision_basis = 'resting_after_verified_sit'
                return None
            kind, basis = 'rise', 'observed_seated_and_rest_elapsed'
        elif at_seat:
            if self.previous_completed == 'rise' and now_ms - self.completed_at_ms < self.ACTIVITY_PAUSE_MS:
                self.decision_basis = 'paused_after_rise'
                return None
            # A located seat and actual past movement/standing evidence justify
            # sitting; boredom/sleepiness are existing Heart compatibility input.
            if self.previous_completed == 'explore' and (boredom >= .25 or sleepiness >= .28):
                kind, basis = 'sit', 'verified_arrival_memory_and_heart'
            elif self.previous_completed == 'rise':
                kind, basis = 'explore', 'verified_rise_and_return_to_home'
            else:
                self.decision_basis = 'no_justified_seat_action'
                return None
        else:
            if o.seated:
                self.decision_basis = 'seated_away_from_seat'
                return None
            if boredom < .25:
                self.decision_basis = 'existing_boredom_threshold_not_reached'
                return None
            if self.previous_completed == 'explore' and now_ms - self.completed_at_ms < self.ACTIVITY_PAUSE_MS:
                self.decision_basis = 'activity_pause'
                return None
            kind, basis = 'explore', 'existing_boredom_threshold_and_standing_at_home'
        if kind in self.failed:
            self.decision_basis = 'motor_failure_memory_safety_block'
            return None
        self.pending = kind
        self.decision_basis = basis
        return kind
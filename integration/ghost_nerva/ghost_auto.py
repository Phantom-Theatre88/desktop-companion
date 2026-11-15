"""Partial PC compatibility port of CoreS3 Ghost Heart and life-state selection.

Six existing HeartState fields are represented. Time-driven recovery and
WAKE_WORD_DETECTED deltas follow HeartEngine.cpp; sleep thresholds follow
BehaviorEngine.cpp. This is NOT complete Ghost, learned behavior, or a new
personality LOCK. Comfort/Social remain unimplemented (separate from mood and
affection). No invented reactions to diagnostic near/quiet/blink commands.
"""
from __future__ import annotations

from dataclasses import dataclass


def clamp01(value: float) -> float:
    return min(1.0, max(0.0, value))


@dataclass
class GhostAutonomy:
    # CoreS3 HeartState defaults, not proposed personality constants.
    mood: float = 0.60
    affection: float = 0.55
    curiosity: float = 0.65
    boredom: float = 0.15
    sleepiness: float = 0.15
    attention: float = 0.50
    elapsed_ms: int = 0
    last_boredom_ms: int = 0
    last_sleepiness_ms: int = 0
    last_meaningful_ms: int = 0
    last_recovery_ms: int = 0
    last_event_ms: int = 0
    last_decision_ms: int = 0
    decision_count: int = 0
    action: str = "idle"
    awake_hold_until_ms: int = 0

    # These baselines mirror CoreS3 first-boot HeartState.
    # Dynamic learned baseline and elapsed-time restore remain unimplemented.
    def _recovery_step(self) -> None:
        for name, baseline, fraction in (
            ("mood", 0.60, 0.12),
            ("curiosity", 0.65, 0.10),
            ("attention", 0.50, 0.18),
        ):
            value = getattr(self, name)
            difference = (baseline - value) * fraction
            setattr(self, name, baseline if abs(difference) <= 0.0005
                    else clamp01(value + difference))

    def wake(self, now_ms: int) -> str:
        """Simulate WAKE_WORD_DETECTED; no microphone or real audio detection.

        CoreS3 habituation uses event history. Only the first wake-word delta
        is modeled; repeated-stimulus attenuation is not yet ported.
        """
        if now_ms < self.elapsed_ms:
            raise ValueError("Ghost time cannot run backwards")
        self.elapsed_ms = now_ms
        self.curiosity = clamp01(self.curiosity + 0.03)
        self.boredom = clamp01(self.boredom - 0.03)
        self.sleepiness = clamp01(self.sleepiness - 0.05)
        self.attention = clamp01(self.attention + 0.10)
        self.last_event_ms = now_ms
        self.last_boredom_ms = now_ms
        self.last_sleepiness_ms = now_ms
        self.last_meaningful_ms = now_ms
        self.awake_hold_until_ms = now_ms + 15_000
        self.last_decision_ms = now_ms
        self.action = "idle"
        return self.action

    def tick(self, now_ms: int) -> str | None:
        """Update six-state partial Heart; emit only semantic action changes."""
        if now_ms < self.elapsed_ms:
            raise ValueError("Ghost time cannot run backwards")
        self.elapsed_ms = now_ms
        while now_ms - self.last_recovery_ms >= 5_000:
            self.last_recovery_ms += 5_000
            if self.last_recovery_ms - self.last_event_ms >= 15_000:
                self._recovery_step()
        while now_ms - self.last_boredom_ms >= 45_000:
            self.boredom = clamp01(self.boredom + 0.015)
            self.last_boredom_ms += 45_000
        while now_ms - self.last_sleepiness_ms >= 30_000:
            self.last_sleepiness_ms += 30_000
            if self.last_sleepiness_ms - self.last_meaningful_ms >= 90_000:
                self.sleepiness = clamp01(self.sleepiness + 0.025)
        if now_ms - self.last_decision_ms < 15_000:
            return None
        self.last_decision_ms = now_ms
        self.decision_count += 1
        if now_ms < self.awake_hold_until_ms:
            next_action = "idle"
        elif self.sleepiness >= 0.42 and self.boredom >= 0.28:
            next_action = "sleep"
        elif self.sleepiness >= 0.28 and self.boredom >= 0.20:
            next_action = "drowsy"
        elif self.boredom >= 0.25:
            next_action = "scan"
        else:
            next_action = "idle"
        if next_action == self.action:
            return None
        self.action = next_action
        return next_action

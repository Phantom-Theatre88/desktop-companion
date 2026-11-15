"""Minimal autonomous Ghost compatibility slice for the NERVA integration.

Only inactivity-driven boredom, fatigue and action selection are carried over
from the existing CoreS3 HeartEngine/BehaviorEngine. This is not a replacement
for the complete Ghost; no new personality constants are introduced.
"""
from __future__ import annotations

from dataclasses import dataclass


@dataclass
class GhostAutonomy:
    elapsed_ms: int = 0
    boredom: float = 0.0
    sleepiness: float = 0.0
    last_boredom_ms: int = 0
    last_sleepiness_ms: int = 0
    last_decision_ms: int = 0
    decision_count: int = 0
    action: str = "idle"
    awake_hold_until_ms: int = 0

    def wake(self, now_ms: int) -> str:
        """Simulated semantic WAKE_WORD stimulus, not a new Ghost policy.

        Mirrors the existing CoreS3 wake-word Heart adjustments and 15 s
        awake hold. Calling near/face detected does NOT wake a sleeping Ghost.
        """
        if now_ms < self.elapsed_ms:
            raise ValueError("Ghost time cannot run backwards")
        self.elapsed_ms = now_ms
        self.boredom = max(0.0, self.boredom - 0.03)
        self.sleepiness = max(0.0, self.sleepiness - 0.05)
        self.last_boredom_ms = now_ms
        self.last_sleepiness_ms = now_ms
        self.awake_hold_until_ms = now_ms + 15_000
        self.last_decision_ms = now_ms
        self.action = "idle"
        return self.action

    def tick(self, now_ms: int) -> str | None:
        """Return a new *semantic intent* only when the autonomous action changes."""
        if now_ms < self.elapsed_ms:
            raise ValueError("Ghost time cannot run backwards")
        self.elapsed_ms = now_ms
        # Match existing HeartEngine C++ inactivity intervals and increments.
        while now_ms - self.last_boredom_ms >= 45_000:
            self.boredom = min(1.0, self.boredom + 0.015)
            self.last_boredom_ms += 45_000
        while now_ms - self.last_sleepiness_ms >= 30_000:
            self.last_sleepiness_ms += 30_000
            if self.last_sleepiness_ms >= 90_000:
                self.sleepiness = min(1.0, self.sleepiness + 0.025)
        if now_ms - self.last_decision_ms < 15_000:
            return None
        self.last_decision_ms = now_ms
        self.decision_count += 1
        # Existing C++ wake-word stimulus guarantees awake for 15 seconds.
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

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
    mood: float = 0.60
    affection: float = 0.55
    curiosity: float = 0.65
    boredom: float = 0.15
    sleepiness: float = 0.15
    attention: float = 0.50
    last_boredom_ms: int = 0
    last_sleepiness_ms: int = 0
    last_decision_ms: int = 0
    decision_count: int = 0
    action: str = "idle"
    # Temporary Ghost Time to NERVA trial path, not a permanent personality rule.
    life_trial_phase: str = "idle"
    life_trial_enabled: bool = False
    awaiting_result: bool = False
    trial_completions: int = 0
    trial_blocked: bool = False

    def on_motion_result(self, kind: str, status: str) -> None:
        if not self.awaiting_result or kind != self.life_trial_phase:
            return
        if status == "completed":
            self.awaiting_result = False
            self.trial_completions += 1
        elif status in ("rejected", "interrupted"):
            self.awaiting_result = False
            self.life_trial_phase = "idle"
            self.trial_blocked = True

    def tick(self, now_ms: int) -> str | None:
        """Return a new *semantic intent* only when the autonomous action changes."""
        if now_ms < self.elapsed_ms:
            raise ValueError("Ghost time cannot run backwards")
        self.elapsed_ms = now_ms
        # Retain the CoreS3 six-state baseline, WITHOUT invented new coefficients.
        # Only boredom and sleepiness drive the compatibility behavior policy.
        # Mood, affection, curiosity, attention remain read-only baselines here.
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
        # Preserve existing boredom/sleepiness thresholds. During the spatial
        # trial, Ghost Time explores after the first 15 s, then uses actual
        # NERVA completion feedback to choose sit/rise (never timed fake success).
        if self.life_trial_enabled and not self.trial_blocked and self.life_trial_phase == "idle" and self.decision_count >= 1:
            self.life_trial_phase = "explore"
            self.awaiting_result = True
            self.action = "explore"
            return "explore"
        if self.life_trial_enabled and self.awaiting_result:
            return None
        if self.life_trial_enabled and self.life_trial_phase == "explore":
            self.life_trial_phase = "sit"
            self.awaiting_result = True
            self.action = "sit"
            return "sit"
        if self.life_trial_enabled and self.life_trial_phase == "sit":
            self.life_trial_phase = "rise"
            self.awaiting_result = True
            self.action = "rise"
            return "rise"
        if self.life_trial_enabled and self.life_trial_phase == "rise":
            self.life_trial_phase = "idle"
            self.action = "idle"
            return "idle"
        # The original Heart compatibility policy stays available outside
        # this provisional exercise. This is not an emotion/Relationship rule.
        # Match the tested C++ behavior thresholds; do not generate new motions.
        if self.sleepiness >= 0.42 and self.boredom >= 0.28:
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
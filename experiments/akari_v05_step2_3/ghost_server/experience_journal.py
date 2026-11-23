"""Experimental durable Memory adapter. Not a formal storage specification.

Records only observed SemanticEvents and immutable entry/exit Heart snapshots.
Never invents an actor, emotion, intent, or successful body motion.
Replays events after restart; power-off Heart recovery and Learning A remain open.
"""
from __future__ import annotations

import json
import sqlite3
from pathlib import Path
from ghost_core import Experience, HeartSnapshot, SemanticEvent


class ExperienceJournal:
    def __init__(self, location: str | Path):
        self.path = Path(location).expanduser()
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self.conn = sqlite3.connect(str(self.path), timeout=5.0)
        self.conn.execute('PRAGMA busy_timeout=5000')
        self.conn.execute('CREATE TABLE IF NOT EXISTS experiences ('
                          'event_id TEXT PRIMARY KEY, payload TEXT NOT NULL)')
        self.conn.commit()

    def close(self) -> None:
        self.conn.close()

    @staticmethod
    def _encode(experience: Experience) -> str:
        e = experience.event
        return json.dumps({
            'event': {'event_id': e.event_id, 'kind': e.kind,
                      'occurred_ms': e.occurred_ms, 'actor_id': e.actor_id},
            'heart_before': vars(experience.heart_before),
            'heart_after': vars(experience.heart_after),
            'interpretation': experience.interpretation,
        }, ensure_ascii=False, separators=(',', ':'))

    @staticmethod
    def _decode(payload: str) -> Experience:
        data = json.loads(payload)
        event = SemanticEvent(**data['event'])
        return Experience(event=event, heart_before=HeartSnapshot(**data['heart_before']),
                          heart_after=HeartSnapshot(**data['heart_after']),
                          interpretation=data['interpretation'])

    def append(self, experience: Experience) -> None:
        """Fail on duplicate event: never quietly overwrite a recorded experience."""
        with self.conn:
            self.conn.execute('INSERT INTO experiences(event_id,payload) VALUES(?,?)',
                              (experience.event.event_id, self._encode(experience)))

    def read_all(self) -> tuple[Experience, ...]:
        # SQLite rowid is insertion order for the current table; IDs themselves
        # are immutable, so re-reading never silently updates interpretation.
        rows = self.conn.execute('SELECT payload FROM experiences ORDER BY rowid')
        return tuple(self._decode(row[0]) for row in rows)
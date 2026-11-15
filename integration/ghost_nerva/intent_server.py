"""Temporary local integration harness: Ghost event -> intent -> NERVA.

It uses the existing Python Ghost core and its Memory/Heart boundaries.  The
event-to-intent mapping below is a *test fixture*, not the finished Behavior
policy or a new personality LOCK.
"""
from __future__ import annotations

import asyncio
import json
import sys
import time
from pathlib import Path

from websockets.asyncio.server import serve

GHOST_SOURCE = Path(__file__).resolve().parents[2] / "experiments" / "ghost_pc_spike"
sys.path.insert(0, str(GHOST_SOURCE))
from ghost_core import Ghost, HeartSnapshot, SemanticEvent  # noqa: E402


def intent_from_test_event(kind: str) -> dict:
    """Test-only semantic intent. No joint angles or animation commands."""
    return {
        "near": {"kind": "attend", "strength": 0.55},
        "quiet": {"kind": "rest", "strength": 0.25},
        "blink": {"kind": "blink", "strength": 1.0},
        "status": {"kind": "idle", "strength": 0.0},
    }[kind]


def envelope(sequence: int, kind: str) -> dict:
    return {"type": "ghost_intent", "version": 1, "sequence": sequence,
            "intent": intent_from_test_event(kind)}


async def main() -> None:
    ghost = Ghost(HeartSnapshot(mood=0.0, comfort=0.5, captured_ms=0))
    clients: set = set()
    sequence = 0
    started = time.monotonic()

    async def connection(ws) -> None:
        clients.add(ws)
        try:
            await ws.send(json.dumps(envelope(sequence, "status")))
            async for _ in ws:
                # Character cannot change Ghost or WORLD through this channel.
                pass
        finally:
            clients.discard(ws)

    async with serve(connection, "127.0.0.1", 8766):
        print("Ghost/NERVA test: near | quiet | blink | status | quit", flush=True)
        while True:
            line = await asyncio.to_thread(sys.stdin.readline)
            if not line or line.strip() == "quit":
                break
            kind = line.strip()
            if kind not in {"near", "quiet", "blink", "status"}:
                print("Use near, quiet, blink, status or quit", flush=True)
                continue
            sequence += 1
            if kind != "status":
                now_ms = int((time.monotonic() - started) * 1000)
                # No invented Heart changes: keep the prior state unchanged.
                ghost.record_event(
                    SemanticEvent(str(sequence), kind, now_ms),
                    ghost.heart.snapshot(),
                )
            msg = json.dumps(envelope(sequence, kind))
            for ws in tuple(clients):
                try:
                    await ws.send(msg)
                except Exception as exc:
                    print(f"Transport failure: {exc}", file=sys.stderr)
            print(msg, flush=True)


if __name__ == "__main__":
    asyncio.run(main())

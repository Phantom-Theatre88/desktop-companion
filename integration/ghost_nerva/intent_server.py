"""Temporary local integration harness: Ghost event -> intent -> NERVA.

It uses the existing Python Ghost core and its Memory/Heart boundaries.  The
event-to-intent mapping below is a *test fixture*, not the finished Behavior
policy or a new personality LOCK.
"""
from __future__ import annotations

import asyncio
import argparse
import json
import sys
import time
from pathlib import Path

from websockets.asyncio.server import serve
from websockets.exceptions import ConnectionClosed

GHOST_SOURCE = Path(__file__).resolve().parents[2] / "experiments" / "ghost_pc_spike"
sys.path.insert(0, str(GHOST_SOURCE))
from ghost_core import Ghost, HeartSnapshot, SemanticEvent  # noqa: E402
from ghost_auto import GhostAutonomy  # noqa: E402


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


def autonomous_envelope(sequence: int, kind: str) -> dict:
    """Unlike the manual test input, this intent comes from Ghost time/state."""
    return {"type": "ghost_intent", "version": 1, "sequence": sequence,
            "intent": {"kind": kind, "strength": 0.6}}


async def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--time-scale", type=float, default=1.0,
                        help="test-only Ghost clock acceleration (default: real time)")
    parser.add_argument("--headless", action="store_true",
                        help="run without console input until stopped")
    args = parser.parse_args()
    if not 1.0 <= args.time_scale <= 60.0:
        parser.error("--time-scale must be between 1 and 60")
    ghost = Ghost(HeartSnapshot(mood=0.0, comfort=0.5, captured_ms=0))
    autonomy = GhostAutonomy()
    clients: set = set()
    sequence = 0
    started = time.monotonic()

    async def broadcast(payload: dict) -> None:
        message = json.dumps(payload)
        for ws in tuple(clients):
            try:
                await ws.send(message)
            except Exception as exc:
                print(f"Transport failure: {exc}", file=sys.stderr)
        print(message, flush=True)

    async def autonomous_loop() -> None:
        nonlocal sequence
        while True:
            await asyncio.sleep(1.0)
            elapsed_ms = int((time.monotonic() - started) * 1000 * args.time_scale)
            new_intent = autonomy.tick(elapsed_ms)
            if new_intent is not None:
                sequence += 1
                await broadcast(autonomous_envelope(sequence, new_intent))

    async def connection(ws) -> None:
        clients.add(ws)
        try:
            # Late joiners must receive the CURRENT Ghost state, not fake idle.
            await ws.send(json.dumps(autonomous_envelope(sequence, autonomy.action)))
            try:
                async for _ in ws:
                    # Character cannot change Ghost or WORLD through this channel.
                    pass
            except ConnectionClosed:
                # The renderer may be closed without a WebSocket close frame.
                pass
        finally:
            clients.discard(ws)

    async with serve(connection, "127.0.0.1", 8766):
        print("Ghost runs autonomously. Local websocket: 127.0.0.1:8766", flush=True)
        task = asyncio.create_task(autonomous_loop())
        try:
            if args.headless:
                await asyncio.Event().wait()
                return
            print("Optional test input: near | quiet | blink | status | quit", flush=True)
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
                    ghost.record_event(SemanticEvent(str(sequence), kind, now_ms), ghost.heart.snapshot())
                await broadcast(envelope(sequence, kind))
        finally:
            task.cancel()
            await asyncio.gather(task, return_exceptions=True)


if __name__ == "__main__":
    asyncio.run(main())

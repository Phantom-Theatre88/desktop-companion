"""AKARI Ghost/NERVA integration harness and local diagnostic transport.

The autonomous decisions are the existing GhostAutonomy compatibility SPIKE,
not the completed five-core Ghost. Viewer inputs are explicitly test-only.
No remote bind, no production commands, no new personality coefficients.
"""
from __future__ import annotations

import argparse
import asyncio
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

TEST_KINDS = frozenset(("near", "quiet", "blink"))
VIEWER_ORIGINS = (None, "http://127.0.0.1:8767", "http://localhost:8767")


def intent_from_test_event(kind: str) -> dict:
    """Test fixture only. Never called as an autonomous Ghost judgment."""
    return {
        "near": {"kind": "attend", "strength": 0.55},
        "quiet": {"kind": "rest", "strength": 0.25},
        "blink": {"kind": "blink", "strength": 1.0},
        "status": {"kind": "idle", "strength": 0.0},  # legacy console API
    }[kind]


def envelope(sequence: int, kind: str) -> dict:
    """Legacy console command contract (preserved for existing tests)."""
    return {"type": "ghost_intent", "version": 1, "sequence": sequence,
            "intent": intent_from_test_event(kind), "source": "console_test"}


def autonomous_envelope(sequence: int, kind: str) -> dict:
    return {"type": "ghost_intent", "version": 1, "sequence": sequence,
            "intent": {"kind": kind, "strength": 0.6},
            "source": "ghost_autonomous_spike"}


def snapshot(sequence: int, ghost: Ghost, autonomy: GhostAutonomy,
             time_scale: float) -> dict:
    """Current *implemented* state, marked by provenance; no invented feelings."""
    heart = ghost.heart.snapshot()
    return {
        "type": "ghost_snapshot", "version": 1, "sequence": sequence,
        "source": "ghost_autonomy_compatibility_spike",
        "clock": {"elapsed_ms": autonomy.elapsed_ms, "test_scale": time_scale},
        "autonomy": {
            "boredom": autonomy.boredom,
            "sleepiness": autonomy.sleepiness,
            "action": autonomy.action,
            "decision_count": autonomy.decision_count,
        },
        "heart_spike": {
            "mood": heart.mood, "comfort": heart.comfort,
            "status": "initial_test_values_not_autonomously_updated",
        },
        "memory_spike": {
            "record_count": len(ghost.memory.recall()), "persistent": False,
            "note": "test events only; no complete experience learning",
        },
        "relationship": {"implemented": False},
    }


def validate_viewer_command(raw: str) -> tuple[str, str] | None:
    """Reject non-test or malformed input; no dynamic attribute or code execution."""
    if len(raw) > 1024:
        return None
    try:
        data = json.loads(raw)
    except (json.JSONDecodeError, TypeError):
        return None
    if not isinstance(data, dict) or data.get("type") != "viewer_test_event":
        return None
    if data.get("version") != 1 or data.get("kind") not in TEST_KINDS:
        return None
    request_id = data.get("request_id")
    if not isinstance(request_id, str) or not (1 <= len(request_id) <= 64):
        return None
    return data["kind"], request_id


async def run_server(*, time_scale: float = 1.0, headless: bool = False,
                     stop_event: asyncio.Event | None = None) -> None:
    if not 1.0 <= time_scale <= 60.0:
        raise ValueError("time_scale must be between 1 and 60")
    ghost = Ghost(HeartSnapshot(mood=0.0, comfort=0.5, captured_ms=0))
    autonomy = GhostAutonomy()
    clients: set = set()
    sequence = 0
    started = time.monotonic()
    send_lock = asyncio.Lock()

    async def broadcast(message: dict) -> None:
        payload = json.dumps(message, ensure_ascii=False)
        for ws in tuple(clients):
            try:
                await ws.send(payload)
            except ConnectionClosed:
                clients.discard(ws)
        print(payload, flush=True)

    async def emit_test(kind: str, source: str, request_id: str | None = None) -> None:
        nonlocal sequence
        async with send_lock:
            sequence += 1
            now_ms = int((time.monotonic() - started) * 1000)
            ghost.record_event(SemanticEvent(f"test-{sequence}", kind, now_ms),
                               ghost.heart.snapshot())
            message = envelope(sequence, kind)
            message["source"] = source
            if request_id is not None:
                message["request_id"] = request_id
            await broadcast(message)
            await broadcast(snapshot(sequence, ghost, autonomy, time_scale))

    async def autonomous_loop() -> None:
        nonlocal sequence
        while True:
            await asyncio.sleep(1.0)
            async with send_lock:
                elapsed_ms = int((time.monotonic() - started) * 1000 * time_scale)
                new_intent = autonomy.tick(elapsed_ms)
                if new_intent is not None:
                    sequence += 1
                    await broadcast(autonomous_envelope(sequence, new_intent))
                await broadcast(snapshot(sequence, ghost, autonomy, time_scale))

    async def connection(ws) -> None:
        clients.add(ws)
        try:
            # Late joiners see real current autonomy, not a fabricated idle state.
            async with send_lock:
                await ws.send(json.dumps(autonomous_envelope(sequence, autonomy.action)))
                await ws.send(json.dumps(snapshot(sequence, ghost, autonomy, time_scale)))
            try:
                async for raw in ws:
                    cmd = validate_viewer_command(raw)
                    if cmd is None:
                        # Existing Godot receiver sends no commands.
                        continue
                    kind, request_id = cmd
                    await emit_test(kind, "viewer_test", request_id)
                    await ws.send(json.dumps({
                        "type": "viewer_test_ack", "version": 1,
                        "request_id": request_id, "accepted": True,
                        "detail": "test stimulus sent; NERVA reception unverified",
                    }))
            except ConnectionClosed:
                pass  # Godot may exit without a close frame.
        finally:
            clients.discard(ws)

    async with serve(connection, "127.0.0.1", 8766, origins=VIEWER_ORIGINS):
        print("Ghost local test server: ws://127.0.0.1:8766", flush=True)
        task = asyncio.create_task(autonomous_loop())
        try:
            if stop_event is not None:
                await stop_event.wait()
            elif headless:
                await asyncio.Future()
            else:
                print("Console test: near | quiet | blink | status | quit", flush=True)
                while True:
                    line = await asyncio.to_thread(sys.stdin.readline)
                    if not line or line.strip() == "quit":
                        break
                    kind = line.strip()
                    if kind not in TEST_KINDS | {"status"}:
                        print("Use near, quiet, blink, status or quit", flush=True)
                    elif kind == "status":
                        await broadcast(snapshot(sequence, ghost, autonomy, time_scale))
                    else:
                        await emit_test(kind, "console_test")
        finally:
            task.cancel()
            await asyncio.gather(task, return_exceptions=True)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--time-scale", type=float, default=1.0,
                        help="test-only time acceleration, 1..60")
    parser.add_argument("--headless", action="store_true",
                        help="run without console input")
    args = parser.parse_args()
    asyncio.run(run_server(time_scale=args.time_scale, headless=args.headless))


if __name__ == "__main__":
    main()

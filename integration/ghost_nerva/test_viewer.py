"""Ghost Viewer v0.1 contract tests (run from repository root)."""
import asyncio
import json
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ghost_auto import GhostAutonomy
from intent_server import (
    TEST_KINDS, autonomous_envelope, snapshot, validate_viewer_command, run_server
)

ROOT = Path(__file__).resolve().parents[2] / "experiments" / "ghost_pc_spike"
sys.path.insert(0, str(ROOT))
from ghost_core import Ghost, HeartSnapshot


class ViewerContractTests(unittest.TestCase):
    def test_reject_unapproved_and_malformed_commands(self):
        for raw in (
            "not json", "[]" , '{"type":"viewer_test_event","version":1,"kind":"sleep","request_id":"1"}',
            '{"type":"viewer_test_event","version":1,"kind":"near","request_id":""}',
            '{"type":"ghost_intent","version":1,"kind":"near","request_id":"1"}',
            '{"type":"viewer_test_event","version":1,"kind":"near","request_id":1}',
            "x"*1025,
        ):
            self.assertIsNone(validate_viewer_command(raw))
        self.assertEqual(validate_viewer_command(
            '{"type":"viewer_test_event","version":1,"kind":"near","request_id":"abc"}'),
            ("near", "abc"))
        self.assertEqual(TEST_KINDS, {"near", "quiet", "blink"})

    def test_snapshot_uses_live_values_and_labels_spikes(self):
        ghost = Ghost(HeartSnapshot(0, .5, 0))
        autonomy = GhostAutonomy()
        autonomy.tick(900_000)
        data = snapshot(4, ghost, autonomy, 60)
        self.assertEqual(data["autonomy"]["sleepiness"], autonomy.sleepiness)
        self.assertEqual(data["autonomy"]["boredom"], autonomy.boredom)
        self.assertEqual(data["autonomy"]["action"], autonomy.action)
        self.assertEqual(data["clock"]["test_scale"], 60)
        self.assertEqual(data["memory_spike"]["record_count"], 0)
        self.assertFalse(data["relationship"]["implemented"])
        self.assertEqual(data["heart_spike"]["status"], "initial_test_values_not_autonomously_updated")

    def test_autonomous_intent_has_distinct_source(self):
        data = autonomous_envelope(3, "drowsy")
        self.assertEqual(data["source"], "ghost_autonomous_spike")
        self.assertNotIn("request_id", data)


class ViewerWebsocketTests(unittest.IsolatedAsyncioTestCase):
    async def test_viewer_and_godot_get_distinct_events(self):
        from websockets.asyncio.client import connect
        stop = asyncio.Event()
        task = asyncio.create_task(run_server(time_scale=60, headless=True, stop_event=stop))
        try:
            # Avoid relying on a background sleep to wait for socket bind.
            for _ in range(80):
                try:
                    viewer = await connect("ws://127.0.0.1:8766", origin="http://127.0.0.1:8767")
                    break
                except OSError:
                    if task.done():
                        raise RuntimeError("Ghost server stopped") from task.exception()
                    await asyncio.sleep(.05)
            else:
                self.fail("Ghost did not bind")
            async with viewer:
                await asyncio.wait_for(viewer.recv(), 3)  # initial autonomous intent
                initial = json.loads(await asyncio.wait_for(viewer.recv(), 3))
                self.assertEqual(initial["type"], "ghost_snapshot")
                await viewer.send(json.dumps({"type": "viewer_test_event", "version": 1,
                                              "kind": "near", "request_id": "v1"}))
                seen = {}
                for _ in range(5):
                    message = json.loads(await asyncio.wait_for(viewer.recv(), 3))
                    seen[message["type"]] = message
                    if "viewer_test_ack" in seen and "ghost_intent" in seen:
                        break
                self.assertEqual(seen["ghost_intent"]["source"], "viewer_test")
                self.assertEqual(seen["ghost_intent"]["intent"]["kind"], "attend")
                self.assertTrue(seen["viewer_test_ack"]["accepted"])
                self.assertFalse("nerva_confirmed" in seen["viewer_test_ack"])
        finally:
            stop.set()
            await asyncio.wait_for(task, 3)


if __name__ == "__main__":
    unittest.main()

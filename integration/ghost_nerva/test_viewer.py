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
        self.assertEqual(TEST_KINDS, {"near", "quiet", "blink", "wake", "touch"})

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
        self.assertEqual(data["heart_spike"]["status"], "separate_memory_boundary_test_only_not_active_heart")

    def test_six_existing_state_fields_are_readable_but_not_eight_invented_scores(self):
        state = GhostAutonomy()
        self.assertEqual(
            (state.mood, state.affection, state.curiosity,
             state.boredom, state.sleepiness, state.attention),
            (.60, .55, .65, .15, .15, .50))
        data = snapshot(1, Ghost(HeartSnapshot(0, .5, 0)), state, 1)
        self.assertEqual(data["autonomy"]["state_origin"], "CoreS3_HeartState_6_partial_port")
        self.assertEqual(data["autonomy"]["pending"], ["comfort", "social"])
        self.assertNotIn("comfort", data["autonomy"])
        self.assertNotIn("social", data["autonomy"])

    def test_touch_changes_mood_affection_without_inventing_comfort(self):
        state = GhostAutonomy()
        state.touch(1000)
        self.assertAlmostEqual(state.mood, .63)
        self.assertAlmostEqual(state.affection, .57)
        self.assertAlmostEqual(state.attention, .54)
        state.tick(31_000)
        self.assertLess(state.mood, .63)
        self.assertAlmostEqual(state.affection, .57)

    def test_wake_restores_coreS3_attention_and_curiosity(self):
        state = GhostAutonomy()
        state.wake(1000)
        self.assertAlmostEqual(state.curiosity, .68)
        self.assertAlmostEqual(state.attention, .60)
        state.tick(31_000)
        self.assertLess(state.curiosity, .68)
        self.assertLess(state.attention, .60)

    def test_existing_wake_response_is_temporary_not_sleep_deletion(self):
        autonomy = GhostAutonomy()
        autonomy.tick(1_800_000)
        self.assertEqual(autonomy.action, "sleep")
        self.assertEqual(autonomy.wake(1_800_000), "idle")
        self.assertEqual(autonomy.tick(1_806_000), None)
        self.assertEqual(autonomy.action, "idle")
        autonomy.tick(1_830_000)
        self.assertEqual(autonomy.action, "sleep")

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
            try:
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
                await viewer.close()
        finally:
            stop.set()
            await asyncio.wait_for(task, 3)


if __name__ == "__main__":
    unittest.main()

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from intent_server import envelope, intent_from_test_event
from ghost_auto import GhostAutonomy
from intent_server import autonomous_envelope


class IntentContractTests(unittest.TestCase):
    def test_no_joint_angles_leave_ghost(self):
        msg = envelope(3, "near")
        self.assertEqual(msg["intent"]["kind"], "attend")
        self.assertEqual(set(msg["intent"]), {"kind", "strength"})

    def test_all_test_intents(self):
        for kind in ("near", "quiet", "blink", "status"):
            self.assertLessEqual(intent_from_test_event(kind)["strength"], 1.0)

    def test_ghost_is_autonomous_without_operator_input(self):
        ghost = GhostAutonomy()
        transitions = []
        for ms in range(0, 1_800_001, 1000):
            event = ghost.tick(ms)
            if event is not None:
                transitions.append(event)
        self.assertIn("drowsy", transitions)
        self.assertIn("sleep", transitions)

    def test_autonomous_messages_contain_intent_only(self):
        message = autonomous_envelope(2, "scan")
        self.assertEqual(message["intent"], {"kind": "scan", "strength": 0.6})

    def test_clock_must_be_monotonic(self):
        ghost = GhostAutonomy()
        ghost.tick(1000)
        with self.assertRaises(ValueError):
            ghost.tick(999)


if __name__ == "__main__":
    unittest.main()

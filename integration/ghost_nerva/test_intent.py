import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from intent_server import envelope, intent_from_test_event


class IntentContractTests(unittest.TestCase):
    def test_no_joint_angles_leave_ghost(self):
        msg = envelope(3, "near")
        self.assertEqual(msg["intent"]["kind"], "attend")
        self.assertEqual(set(msg["intent"]), {"kind", "strength"})

    def test_all_test_intents(self):
        for kind in ("near", "quiet", "blink", "status"):
            self.assertLessEqual(intent_from_test_event(kind)["strength"], 1.0)


if __name__ == "__main__":
    unittest.main()

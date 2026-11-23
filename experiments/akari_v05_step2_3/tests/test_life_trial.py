import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parents[1] / "akari_motion_test" / "ghost_server"))
from ghost_auto import GhostAutonomy

class LifeTrial(unittest.TestCase):
    def test_baseline_policy_preserved_when_opt_out(self):
        x=GhostAutonomy()
        self.assertEqual(x.tick(15000), None)
        self.assertEqual(x.action, "idle")
    def test_requires_real_completion(self):
        x=GhostAutonomy(life_trial_enabled=True)
        self.assertEqual(x.tick(15000), "explore")
        self.assertIsNone(x.tick(30000))
        x.on_motion_result("explore", "applied_frame")
        self.assertIsNone(x.tick(45000))
        x.on_motion_result("explore", "completed")
        self.assertEqual(x.tick(60000), "sit")
        self.assertIsNone(x.tick(75000))
        x.on_motion_result("sit", "completed")
        self.assertEqual(x.tick(90000), "rise")
        x.on_motion_result("rise", "completed")
        self.assertEqual(x.tick(105000), "idle")
        self.assertEqual(x.trial_completions, 3)
    def test_failure_not_accepted_as_success(self):
        x=GhostAutonomy(life_trial_enabled=True)
        x.tick(15000)
        x.on_motion_result("explore", "rejected")
        self.assertEqual(x.trial_completions, 0)
        self.assertFalse(x.awaiting_result)
        self.assertEqual(x.life_trial_phase, "idle")
    def test_character_has_procedural_spatial_controls(self):
        files = Path(__file__).parents[1] / "akari_motion_test" / "scripts"
        body=(files / "nerva_controller.gd").read_text()
        receiver=(files / "ghost_intent_receiver.gd").read_text()
        for token in ("_advance_life", "_apply_life_pose", "motion_root.position.x", "_sit_amount", "_walk_cycle", "_hand_ratio", "motion_finished"):
            self.assertIn(token, body)
        self.assertIn('"completed"', receiver)

if __name__ == "__main__": unittest.main()
import unittest
from pathlib import Path
class VisibleBodyTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        p=Path(__file__).resolve().parents[1]/'akari_motion_test/scripts/nerva_controller.gd'
        cls.src=p.read_text()
    def test_visible_motion_uses_existing_ghost_intents(self):
        for k in ('idle','scan','drowsy','sleep'):
            self.assertIn(f'_ghost_intent == "{k}"',self.src)
    def test_arm_elbow_torso_motion_not_only_head(self):
        for token in ('left_extra = -38.0 * g','right_extra = 38.0 * g','right_elbow = 39.0 * g','body_lean = 3.5 * g','_rot("spine",','_rot("rightLowerArm",'):
            self.assertIn(token,self.src)
    def test_motion_blends_in_and_out(self):
        self.assertIn('_gesture_blend = lerpf(',self.src)
        self.assertIn('gesture_goal = sin(PI * _gesture_timer / 3.2)',self.src)
    def test_no_claim_of_walking_or_seating(self):
        self.assertNotIn('func walk(',self.src)
        self.assertNotIn('func sit(',self.src)
if __name__=='__main__':unittest.main()
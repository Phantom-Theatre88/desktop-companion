"""Signed-facing regression for AKARI's +Z-facing GLB; not a Godot visual pass."""
from pathlib import Path
import math, unittest
C = (Path(__file__).parents[1] / 'akari_motion_test/scripts/nerva_controller.gd').read_text()

class Facing(unittest.TestCase):
    def test_negative_x_is_negative_yaw_for_plus_z_avatar(self):
        direction=(-0.65,0.0)
        yaw=math.atan2(direction[0],direction[1]);self.assertAlmostEqual(yaw,-math.pi/2)
        # yaw about world Y transforms +Z to (sin(yaw),0,cos(yaw)).
        self.assertLess(math.sin(yaw), -0.999)
    def test_old_reverse_yaw_removed(self):
        self.assertNotIn('motion_root.rotation.y = 1.40',C)
        self.assertIn('var travel_yaw := atan2(travel.x, travel.z)',C)
    def test_turn_then_move_then_turn(self):
        self.assertIn('if _life_phase < TURN_SECONDS:',C)
        self.assertIn('elif _life_phase < TURN_SECONDS + WALK_SECONDS:',C)
        self.assertIn('motion_root.rotation.y = travel_yaw',C)
        self.assertIn('motion_root.position.x = _life_target_x',C)
    def test_translation_and_gait_share_dx(self):
        self.assertIn('var dx: float = motion_root.position.x - _last_walk_x',C)
        self.assertIn('_gait_distance += absf(dx)',C)
        self.assertIn('_walk_cycle = _gait_distance * TAU / 0.27',C)
if __name__=='__main__':unittest.main()
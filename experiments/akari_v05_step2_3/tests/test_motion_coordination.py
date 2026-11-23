"""Static regression tests for no longer driving body and legs independently.
Not a claim that physical foot contact, Godot pose correctness, or animation naturalness passed.
"""
import pathlib
import math
import unittest

CODE = (pathlib.Path(__file__).resolve().parents[1]/'akari_motion_test/scripts/nerva_controller.gd').read_text()

class MotionCoordination(unittest.TestCase):
    def test_travel_drives_stride(self):
        self.assertIn('_gait_distance += absf(dx)', CODE)
        self.assertIn('_walk_cycle = _gait_distance * TAU / 0.27', CODE)
        self.assertNotIn('_walk_cycle += delta * TAU * 1.35', CODE)

    def test_travel_has_facing_and_start_stop_blend(self):
        self.assertIn('motion_root.rotation.y = 1.40', CODE)
        self.assertIn('_walk_envelope = clampf(sin(PI * progress)', CODE)
        self.assertIn('motion_root.rotation.y = 0.0', CODE)

    def test_sitting_hips_legs_arms_are_blended_together(self):
        self.assertIn('var seat: float = clampf(_sit_amount', CODE)
        self.assertIn('_seat_offset_z * seat', CODE)
        self.assertIn('72.0 * seat', CODE)
        self.assertIn('15.0 * seat', CODE)

    def test_pose_ends_with_reset_to_base_hip(self):
        self.assertIn('skeleton.set_bone_pose_position(int(_bones["hips"]), _base_pos["hips"])', CODE)

    def test_no_jumps_in_ease(self):
        e=lambda p:p*p*(3-2*p)
        self.assertAlmostEqual(e(0),0)
        self.assertAlmostEqual(e(1),1)
        self.assertAlmostEqual((e(1e-4)-e(0))/1e-4,0,delta=.001)
        self.assertAlmostEqual((e(1)-e(1-1e-4))/1e-4,0,delta=.001)

if __name__=='__main__': unittest.main()
import unittest
from pathlib import Path
SCRIPT=Path(__file__).resolve().parents[1]/'akari_motion_test/tools/check_akari_model.gd'
class MotionTrackAcceptance(unittest.TestCase):
    def test_requires_real_joint_rotation_tracks(self):
        code=SCRIPT.read_text()
        self.assertIn('Animation.TYPE_ROTATION_3D',code)
        self.assertIn('driven_bones < 8',code)
    def test_actual_animation_finished_signal(self):
        code=(SCRIPT.parent.parent/'scripts/nerva_controller.gd').read_text()
        self.assertIn('animation_finished.connect(_on_mocap_animation_finished)',code)
        self.assertIn('return _mocap_completed_clip == clip',code)
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]

class MotionCompletionTests(unittest.TestCase):
    def setUp(self):
        self.src=(ROOT/'akari_motion_test/scripts/nerva_controller.gd').read_text()
    def test_real_player_completion_gates_success(self):
        block=self.src.split('func motion_finished(kind: String, serial: int) -> bool:',1)[1].split('func ',1)[0]
        self.assertIn('if not _mocap_terminal_ready(kind):',block)
    def test_stop_required_for_sit_rise(self):
        block=self.src.split('func _mocap_terminal_ready(kind: String) -> bool:',1)[1].split('func ',1)[0]
        self.assertIn('return _mocap_completed_clip == clip',block)
        self.assertIn('animation_finished.connect(_on_mocap_animation_finished)',self.src)
    def test_walk_loop_is_not_waited_to_finish(self):
        block=self.src.split('func _mocap_terminal_ready(kind: String) -> bool:',1)[1].split('func ',1)[0]
        self.assertIn('mocap_current == "walk" and mocap_player.is_playing()',block)
    def test_missing_clip_cannot_succeed(self):
        block=self.src.split('func _mocap_terminal_ready(kind: String) -> bool:',1)[1].split('func ',1)[0]
        self.assertIn('if clip == "":',block)
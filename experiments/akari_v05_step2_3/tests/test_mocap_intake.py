import unittest, pathlib, sys
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'akari_motion_test/tools'))
from audit_mocap_glb import audit
class Intake(unittest.TestCase):
 def test_character_glb_is_not_motion_library(self):
  with self.assertRaisesRegex(ValueError,'No glTF animations'):
   audit(ROOT/'akari_motion_test/avatar/akari_test.glb')
 def test_empty_motion_folders_not_claimed_populated(self):
  self.assertEqual(list((ROOT/'akari_motion_test/motions/source').glob('*.glb')),[])
if __name__=='__main__':unittest.main()
"""Actual AKARI GLB seated leg separation and time-continuity regression."""
import json,math,sys,unittest,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]/'akari_motion_test'
sys.path.insert(0,str(ROOT/'tools'))
from audit_leg_separation import audit_leg_separation_bytes
from audit_mocap_pose import sample_pose
from install_cc0_motion import glb_read
class TestSeatedLegAlignment(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.bones=json.loads((ROOT/'avatar/vrm_expression_map.json').read_text())['bones']
        cls.payload=(ROOT/'avatar/akari_animated.glb').read_bytes()
        cls.js,cls.blob=glb_read(cls.payload)
    def test_real_model_has_side_clearance(self):
        a=audit_leg_separation_bytes(self.payload,self.bones)
        for clip in ('sit','seated_idle','rise'):
            self.assertGreaterEqual(a[clip]['min_left_minus_right_toe_x_m'],.10)
            self.assertGreaterEqual(a[clip]['min_toe_height_m'],-.002)
    def test_boundaries_both_feet_stay_same_place(self):
        endpoints=[('sit',1,'seated_idle',0),('seated_idle',1,'rise',0)]
        for aa,t,bb,u in endpoints:
            pa=sample_pose(self.js,self.blob,self.bones,aa,t)
            pb=sample_pose(self.js,self.blob,self.bones,bb,u)
            for key in ('leftToes','rightToes','hips'):
                self.assertLess(math.dist(pa[key],pb[key]),.003,(aa,bb,key))
    def test_motor_calibration_has_no_single_frame_jump(self):
        lateral=json.loads((ROOT/'tools/leg_lateral_calibration.json').read_text())
        pinned=json.loads((ROOT/'tools/contact_calibration.json').read_text())
        self.assertEqual(lateral['original_sha256'],hashlib.sha256((ROOT/'avatar/akari_test.glb').read_bytes()).hexdigest())
        for clip in ('sit','rise'):
            arr=lateral['values'][clip]['angles']
            old=pinned['clips'][clip]['legs']['right']
            combined=[[old['UpperLeg'][i]+x[0],x[1],old['LowerLeg'][i]+x[2],old['Foot'][i]+x[3]] for i,x in enumerate(arr)]
            self.assertEqual(len(combined),len(lateral['values'][clip]['times']))
            self.assertLess(max(abs(combined[i+1][j]-combined[i][j]) for i in range(len(combined)-1) for j in range(4)),5.6)
        for x in lateral['values']['sit']['angles'][0]:self.assertLess(abs(x),.002)
        for x in lateral['values']['rise']['angles'][-1]:self.assertLess(abs(x),.002)
        for a,b in zip(lateral['values']['sit']['angles'][-1],lateral['values']['rise']['angles'][0]):self.assertLess(abs(a-b),.002)
if __name__=='__main__':unittest.main()
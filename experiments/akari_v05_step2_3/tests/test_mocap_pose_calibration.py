"""Regression against screenshot-proven wrong-side arm retargeting."""
import json, math, struct, sys, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1] / 'akari_motion_test'
sys.path.insert(0,str(ROOT/'tools'))
from install_cc0_motion import glb_read,glb_pack,quat_mul,quat_norm
from audit_mocap_pose import audit_pose_bytes, CORE, SAMPLES
class TruePostureTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.bones=json.loads((ROOT/'avatar/vrm_expression_map.json').read_text())['bones']
        cls.payload=(ROOT/'avatar/akari_animated.glb').read_bytes()
    def test_real_mocap_neutral_not_duke_walk(self):
        report=audit_pose_bytes(self.payload,self.bones)
        self.assertEqual(set(report),set(CORE))
        for clip,frames in report.items():
            self.assertEqual(len(frames),len(SAMPLES))
            for frame in frames:
                self.assertLess(frame['left_wrist_minus_shoulder_m'],0.12,(clip,frame))
                self.assertLess(frame['right_wrist_minus_shoulder_m'],0.12,(clip,frame))
    def test_reversed_arms_are_rejected_even_if_glb_tracks_exist(self):
        js,b=glb_read(self.payload);b=bytearray(b)
        names={n.get('name'):i for i,n in enumerate(js['nodes'])}
        anim=next(x for x in js['animations'] if x['name']=='idle')
        for side,angle in [('left',158.),('right',-158.)]:
            target=names[self.bones[side+'UpperArm']]
            channels=[x for x in anim['channels'] if x['target'].get('node')==target and x['target'].get('path')=='rotation']
            self.assertEqual(len(channels),1)
            acc=js['accessors'][anim['samplers'][channels[0]['sampler']]['output']]
            view=js['bufferViews'][acc['bufferView']]
            offset=view.get('byteOffset',0)+acc.get('byteOffset',0)
            stride=view.get('byteStride',16)
            half=math.radians(angle)/2.
            zrot=[0.,0.,math.sin(half),math.cos(half)]
            for idx in range(acc['count']):
                pos=offset+idx*stride
                q=struct.unpack_from('<4f',b,pos)
                altered=quat_norm(quat_mul(q,zrot))
                struct.pack_into('<4f',b,pos,*altered)
        wrong=glb_pack(js,bytes(b))
        with self.assertRaisesRegex(ValueError,'hands-up'):audit_pose_bytes(wrong,self.bones)
    def test_original_model_kept_pristine(self):
        import hashlib
        original=(ROOT/'avatar/akari_test.glb').read_bytes()
        self.assertEqual(hashlib.sha256(original).hexdigest(),'2a7fcc66fcdb9a76944a47c666f79911f5f24cd0256685dd7f8c3e66a314b33d')
if __name__=='__main__':unittest.main()
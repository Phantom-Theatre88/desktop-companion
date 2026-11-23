"""Real GLB2 geometry-preservation tests using synthetic CC0-shaped animation assets.
Does NOT assert that the third-party download or Godot rendering succeeded.
"""
import importlib.util, json, struct, tempfile, unittest, zipfile
from unittest.mock import patch
from pathlib import Path

BASE = Path(__file__).resolve().parents[1] / 'akari_motion_test'
spec = importlib.util.spec_from_file_location('mocap', BASE/'tools/install_cc0_motion.py')
mocap = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mocap)


class MotionBridgeTests(unittest.TestCase):
    def test_quaternion_rest_preservation(self):
        q=[0.1,0.2,0.3,0.9]
        v=mocap.quat_norm(mocap.quat_mul(q,mocap.quat_mul(mocap.quat_inv(q),q)))
        self.assertAlmostEqual(sum(x*x for x in v),1,places=5)
        self.assertEqual(mocap.choose_motion_name('Walk.glb'),'walk')
        self.assertEqual(mocap.choose_motion_name('Sit Down.glb'),'sit')
        self.assertEqual(mocap.choose_motion_name('Stand Up.glb'),'rise')

    def synthetic_source(self,name):
        names=['Hips','Spine','Chest','Neck','Head','LeftShoulder','RightShoulder','LeftUpperArm','RightUpperArm',
               'LeftLowerArm','RightLowerArm','LeftHand','RightHand','LeftUpperLeg','RightUpperLeg',
               'LeftLowerLeg','RightLowerLeg','LeftFoot','RightFoot']
        nodes=[{'name':n,'rotation':[0,0,0,1],'translation':[0,1,0] if n=='Hips' else [0,0,0]} for n in names]
        js={'asset':{'version':'2.0'},'buffers':[{'byteLength':0}],'nodes':nodes,'scenes':[{'nodes':list(range(len(nodes)))}],'scene':0}
        channels=[]
        for i in range(len(nodes)):
            channels.append({'node':i,'times':[0.0,0.5,1.0], 'values':[[0,0,0,1],[0.1,0,0,0.994987],[0,0,0,1]]})
        if i==0:pass
        binary=mocap.add_animation(js,b'',name,channels)
        js['buffers'][0]['byteLength']=len(binary)
        return mocap.glb_pack(js,binary)

    def test_build_animated_model_without_modifying_original(self):
        original=BASE/'avatar/akari_test.glb'
        before=original.read_bytes()
        with tempfile.TemporaryDirectory() as tmp:
            tmp=Path(tmp);bundle=tmp/'bundle.zip';dest=tmp/'retargeted.glb'
            with zipfile.ZipFile(bundle,'w') as z:
                for name in ['Walk','Sit Down','Stand Up']:
                    z.writestr(name+'.glb',self.synthetic_source(name))
            with patch.object(mocap, "check_bundle", return_value=None):
                report=mocap.build(bundle,dest,original)
            gltf,_=mocap.glb_read(dest.read_bytes())
            self.assertTrue({'walk','sit','rise'} <= {a['name'] for a in gltf['animations']})
            self.assertEqual(len(gltf['meshes']),3)
            self.assertEqual(len(gltf['skins']),3)
            self.assertEqual(original.read_bytes(),before)
            self.assertTrue((tmp/'akari_animated.provenance.json').is_file())
            for a in gltf['animations']:
                self.assertGreaterEqual(len(a['channels']),10)
        
    def test_missing_required_clip_refuses_export(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp=Path(tmp);bundle=tmp/'bundle.zip';dest=tmp/'akari_animated.glb'
            with zipfile.ZipFile(bundle,'w') as z:
                z.writestr('Walk.glb',self.synthetic_source('walk'))
            with patch.object(mocap, 'check_bundle', return_value=None):
                with self.assertRaisesRegex(ValueError,'Missing mandatory'):
                    mocap.build(bundle,dest,BASE/'avatar/akari_test.glb')
            self.assertFalse(dest.exists())

    def test_motor_keeps_ghost_ownership(self):
        motor=(BASE/'scripts/nerva_controller.gd').read_text()
        main=(BASE/'scripts/main.gd').read_text()
        launcher=(BASE/'起動_AKARI.command').read_text()
        self.assertIn('func bind_mocap(',motor)
        self.assertIn('_start_mocap(kind)',motor)
        self.assertIn('if mocap_enabled:\n        return',motor)
        self.assertIn('bind_mocap(mocap_player)',main)
        self.assertIn('install_cc0_motion.py',launcher)
        self.assertIn('res://avatar/akari_animated.glb',main)


if __name__=='__main__':unittest.main()
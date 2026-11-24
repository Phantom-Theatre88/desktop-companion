"""Actual Kim-supplied CC0 GLB retarget acceptance, not simulated/source-only.
Does not claim Godot rendering or physical foot/seat contact validation.
"""
import hashlib, importlib.util, json, math, struct, tempfile, unittest, zipfile
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
ASSET=ROOT/'akari_motion_test'
SRC=ASSET/'motions/source/KinemIQ_Free_Animations_CC0_GLB.zip'
ORIGINAL=ASSET/'avatar/akari_test.glb'
ANIMATED=ASSET/'avatar/akari_animated.glb'
spec=importlib.util.spec_from_file_location('mocap_akari',ASSET/'tools/install_cc0_motion.py')
mod=importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

@unittest.skipUnless(SRC.is_file() and ANIMATED.is_file() and ORIGINAL.is_file(),
                     'requires actual locally supplied CC0 archive and AKARI models')
class ActualArchive(unittest.TestCase):
    def test_uploaded_zip_contains_35_source_animations(self):
        mod.check_bundle(SRC)
        self.assertEqual(hashlib.sha256(SRC.read_bytes()).hexdigest(),mod.USER_SUPPLIED_CC0_SHA256)
        with zipfile.ZipFile(SRC) as z:
            clips=[x.filename for x in z.infolist() if x.filename.endswith('.glb')
                   and '/GLB/' in x.filename and not x.filename.startswith('__MACOSX/')]
            self.assertEqual(len(clips),35)
            for name in ('Walk_forwards','Action_sit_down','Action_stand_up_from_sitting','Idle_sitting'):
                path=next(x for x in clips if x.endswith('/'+name+'.glb'))
                js,_=mod.glb_read(z.read(path))
                self.assertGreaterEqual(len(js['animations'][0]['channels']),40)

    def test_actual_retarget_has_15_joint_driven_clips(self):
        self.assertTrue(ANIMATED.is_file(), 'ship actual pre-generated AKARI GLB')
        js,blob=mod.glb_read(ANIMATED.read_bytes())
        base,_=mod.glb_read(ORIGINAL.read_bytes())
        self.assertEqual(len(js['skins']),len(base['skins']))
        self.assertEqual(len(js['meshes']),len(base['meshes']))
        clips={a['name']:a for a in js['animations']}
        self.assertGreaterEqual(len(clips),15)
        for name in ['walk','sit','rise','seated_idle','idle','talk_idle','wait']:
            self.assertIn(name,clips)
            armature_rotation_count=sum(c['target']['path']=='rotation' for c in clips[name]['channels'])
            self.assertGreaterEqual(armature_rotation_count,45)
            for ch in clips[name]['channels']:
                sampler=clips[name]['samplers'][ch['sampler']]
                times=mod.read_accessor(js,blob,sampler['input'])
                values=mod.read_accessor(js,blob,sampler['output'])
                self.assertEqual(len(times),len(values))
                self.assertTrue(all(times[i][0] < times[i+1][0] for i in range(len(times)-1)))
                self.assertTrue(all(all(math.isfinite(x) for x in v) for v in values))
                if ch['target']['path']=='rotation':
                    self.assertTrue(all(abs(math.sqrt(sum(x*x for x in v))-1)<0.01 for v in values))

    def test_pelvis_lowered_and_continuous_through_seat(self):
        js,blob=mod.glb_read(ANIMATED.read_bytes())
        hips=next(i for i,n in enumerate(js['nodes']) if n.get('name')=='J_Bip_C_Hips')
        def pelvis(name):
            a=next(a for a in js['animations'] if a['name']==name)
            ch=next(c for c in a['channels'] if c['target']['node']==hips and c['target']['path']=='translation')
            return mod.read_accessor(js,blob,a['samplers'][ch['sampler']]['output'])
        rest,sit,hold,rise=(pelvis(n) for n in ['idle','sit','seated_idle','rise'])
        self.assertGreater(sit[0][1]-sit[-1][1],0.25)
        self.assertGreater(rise[-1][1]-rise[0][1],0.25)
        def dist(a,b):return math.sqrt(sum((a[i]-b[i])**2 for i in range(3)))
        self.assertLess(dist(sit[-1],hold[0]),0.012)
        self.assertLess(dist(hold[-1],rise[0]),0.012)
        self.assertLess(dist(rise[-1],rest[0]),0.012)
        self.assertLess(max(abs(v[2]-rest[0][2]) for v in pelvis('walk')),0.01)

    def test_source_never_overwritten(self):
        self.assertEqual(hashlib.sha256(ORIGINAL.read_bytes()).hexdigest(),
                         '2a7fcc66fcdb9a76944a47c666f79911f5f24cd0256685dd7f8c3e66a314b33d')
        prov=json.loads((ASSET/'avatar/akari_animated.provenance.json').read_text())
        self.assertEqual(prov['source_status'],'user_supplied_audited_zip')
        self.assertLess(prov['seat_continuity']['sit_hold_continuity_m'],0.012)

    def test_real_zip_rebuild_is_deterministic_in_animation_structure(self):
        with tempfile.TemporaryDirectory() as temp:
            report=mod.build(SRC,Path(temp)/'out.glb',ORIGINAL)
            self.assertGreaterEqual(len(report['animations']),15)
            self.assertEqual(report['seat_continuity']['enabled'],True)
            j,_=mod.glb_read(Path(temp,'out.glb').read_bytes())
            self.assertEqual(sorted(a['name'] for a in j['animations']),
                             sorted(a['name'] for a in mod.glb_read(ANIMATED.read_bytes())[0]['animations']))

if __name__ == '__main__':unittest.main()
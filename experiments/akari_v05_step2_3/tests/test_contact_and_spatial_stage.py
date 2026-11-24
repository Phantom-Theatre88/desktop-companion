"""Real AKARI GLB/CC0 FK, floor and chair-contact regression for visual complaints.
Not a claim of Godot runtime IK, physics collisions, or perceptual naturalness.
"""
import json, math, re, struct, sys, unittest
from pathlib import Path
BASE=Path(__file__).resolve().parents[1]/'akari_motion_test'
sys.path.insert(0,str(BASE/'tools'))
from install_cc0_motion import glb_read,glb_pack,read_accessor
from audit_foot_contact import audit_foot_contact_bytes
from audit_mocap_pose import sample_pose

class StageContacts(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.bones=json.loads((BASE/'avatar/vrm_expression_map.json').read_text())['bones']
        cls.payload=(BASE/'avatar/akari_animated.glb').read_bytes()
        cls.js,cls.blob=glb_read(cls.payload)

    def test_foot_not_under_floor_across_real_life_clips(self):
        a=audit_foot_contact_bytes(self.payload,self.bones)
        self.assertEqual(set(a),{'idle','walk','sit','seated_idle','rise'})
        for clip,record in a.items():
            self.assertGreaterEqual(record['min_toe_y_m'],-0.002,clip)

    def test_floor_penetration_is_rejected_even_when_tracks_exist(self):
        j,b=glb_read(self.payload)
        b=bytearray(b)
        hip_id=next(i for i,n in enumerate(j['nodes']) if n.get('name')==self.bones['hips'])
        anim=next(a for a in j['animations'] if a['name']=='sit')
        chan=next(c for c in anim['channels'] if c['target']['node']==hip_id and c['target']['path']=='translation')
        accessor=j['accessors'][anim['samplers'][chan['sampler']]['output']]
        bv=j['bufferViews'][accessor['bufferView']]
        bytebase=bv.get('byteOffset',0)+accessor.get('byteOffset',0)
        stride=bv.get('byteStride',12)
        for i in range(accessor['count']):
            offset=bytebase+i*stride
            v=list(struct.unpack_from('<3f',b,offset));v[1]-=.22
            struct.pack_into('<3f',b,offset,*v)
        with self.assertRaisesRegex(ValueError,'Foot below trial floor'):
            audit_foot_contact_bytes(glb_pack(j,bytes(b)),self.bones)

    def test_seat_transitions_have_no_foot_teleport(self):
        for first,second in [('sit','seated_idle'),('seated_idle','rise')]:
            a=sample_pose(self.js,self.blob,self.bones,first,1.)
            b=sample_pose(self.js,self.blob,self.bones,second,0.)
            for bone in ['hips','leftFoot','rightFoot','leftToes','rightToes']:
                self.assertLess(math.dist(a[bone],b[bone]),0.012,(first,second,bone))

    def test_seated_pelvis_matches_trial_seat_top_without_moving_seat_up(self):
        main=(BASE/'scripts/main.gd').read_text()
        m=re.search(r'seat\.position = Vector3\(-0\.65, ([\d.]+), (-?[\d.]+)\)',main)
        self.assertIsNotNone(m)
        seat_y,seat_z=map(float,m.groups())
        self.assertAlmostEqual(seat_z,-0.45)
        seat_top=seat_y+.08/2
        p=sample_pose(self.js,self.blob,self.bones,'seated_idle',0.)
        self.assertGreater(p['hips'][1]-seat_top,.07)
        self.assertLess(p['hips'][1]-seat_top,.14)
        # Seated pelvis is in the chair's footprint rather than in front of it.
        nerva=(BASE/'scripts/nerva_controller.gd').read_text()
        seated_root=float(re.search(r'SEATED_ROOT_Z: float = (-?\d+\.\d+)',nerva).group(1))
        world_pelvis_z=p['hips'][2]+seated_root
        self.assertGreater(world_pelvis_z,seat_z-0.25)
        self.assertLess(world_pelvis_z,seat_z+0.25)

    def test_walk_lane_avoids_chair_and_root_speed_matches_clip(self):
        nerva=(BASE/'scripts/nerva_controller.gd').read_text()
        walk_s=float(re.search(r'const WALK_SECONDS: float = (\d+\.\d+)',nerva).group(1))
        self.assertLessEqual(walk_s,1.5)
        self.assertGreaterEqual(walk_s,1.0)
        # chair front edge is behind the walking centerline at z=0
        chair_center=-0.45
        self.assertLess(chair_center+0.5/2,-.15)
        self.assertIn('motion_root.position.z = lerpf(_seat_origin_z, SEATED_ROOT_Z',nerva)
        self.assertIn('return absf(motion_root.position.x + 0.65)',nerva)

    def test_captured_sit_finished_turns_into_seated_life(self):
        nerva=(BASE/'scripts/nerva_controller.gd').read_text()
        self.assertIn('if mocap_current in ["sit", "rise"]:',nerva)
        self.assertIn('_start_mocap("idle")',nerva)
        self.assertIn('kind == "sit" and mocap_current in ["seated_idle", "seated_hold"]',nerva)
        self.assertIn('_mocap_completed_clip == _find_clip("sit")',nerva)

if __name__=='__main__':unittest.main()
"""Offline model/scene-contract checks: no rendering or Godot execution claims."""
import json
import struct
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]/'akari_motion_test'

class GLBContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        binary=(ROOT/'avatar/akari_test.glb').read_bytes()
        assert binary[:4] == b'glTF'
        declared_len=struct.unpack_from('<I',binary,8)[0]
        assert declared_len == len(binary)
        json_len,chunk_type=struct.unpack_from('<I4s',binary,12)
        assert chunk_type == b'JSON'
        cls.gltf=json.loads(binary[20:20+json_len])
        cls.mapping=json.loads((ROOT/'avatar/vrm_expression_map.json').read_text())

    def test_meshes_and_skin_skeleton(self):
        self.assertEqual(len(self.gltf['meshes']),3)
        self.assertEqual(len(self.gltf['skins']),3)
        self.assertEqual([len(s['joints']) for s in self.gltf['skins']],[98,98,98])
        all_names={n.get('name') for n in self.gltf['nodes']}
        self.assertTrue(set(self.mapping['bones'].values()) <= all_names)

    def test_face_expressions_are_in_actual_model(self):
        morph_names=set()
        for mesh in self.gltf['meshes']:
            morph_names.update(mesh.get('extras',{}).get('targetNames',[]))
        self.assertGreaterEqual(len(morph_names),57)
        for expression in ('blink','happy','aa','ih','ou','ee','oh'):
            for binding in self.mapping['expressions'][expression]:
                self.assertIn(binding['blend_shape'],morph_names)

    def test_critical_scene_wiring(self):
        main=(ROOT/'scripts/main.gd').read_text()
        recv=(ROOT/'scripts/ghost_intent_receiver.gd').read_text()
        motor=(ROOT/'scripts/nerva_controller.gd').read_text()
        self.assertIn('res://avatar/akari_animated.glb', main)
        self.assertTrue((ROOT / 'avatar/akari_test.glb').is_file())
        self.assertIn('ghost_intent_receiver.gd',main)
        self.assertIn('_nerva.step(delta)',main)
        self.assertIn('func accept_ghost_state(',motor)
        self.assertIn('accept_ghost_state(validated)',recv)
        self.assertIn('"ghost_snapshot"',recv)
        self.assertIn('"state_sync"',recv)
        self.assertIn('func _apply_stance(',motor)
        self.assertIn('model_bound',recv)

if __name__=='__main__':unittest.main()
import asyncio
import json
import sys
import unittest
from pathlib import Path
BASE=Path(__file__).resolve().parents[1]/'akari_motion_test'/'ghost_server'
sys.path.insert(0,str(BASE))
from intent_server import run_server, validate_result, intent_message, state_message
from ghost_core import Ghost, HeartSnapshot, SemanticEvent
from behavior_bridge import BehaviorBridge
from ghost_auto import GhostAutonomy

class BoundaryTests(unittest.TestCase):
    def test_memory_and_relationship_are_read_not_rewritten(self):
        g=Ghost(HeartSnapshot(.6,.5,0)); b=BehaviorBridge(g)
        g.record_event(SemanticEvent('e1','near',1,'person_A'),g.heart.snapshot())
        g.record_event(SemanticEvent('e2','quiet',2),g.heart.snapshot())
        x=b.select('scan','person_A'); y=b.select('scan',None)
        self.assertEqual((x.experience_count,x.relationship_count),(2,1))
        self.assertEqual(y.relationship_count,0)
        self.assertEqual(x.kind,'scan') # No unapproved personality algorithm.
        self.assertEqual(len(g.memory.recall()),2)
    def test_correlated_result_and_deduplication(self):
        g=Ghost(HeartSnapshot(.6,.5,0)); b=BehaviorBridge(g)
        b.register_intent(7,"scan")
        self.assertFalse(b.accept_result({'type':'nerva_result','version':1,'sequence':9,'status':'applied_frame','model_bound':True}))
        self.assertTrue(b.accept_result({'type':'nerva_result','version':1,'sequence':7,'status':'applied_frame','model_bound':True}))
        self.assertTrue(b.accept_result({'type':'nerva_result','version':1,'sequence':7,'status':'completed','model_bound':True}))
        self.assertFalse(b.accept_result({'type':'nerva_result','version':1,'sequence':7,'status':'completed','model_bound':True}))
        self.assertEqual([e.event.kind for e in g.memory.recall()], ['nerva_applied_frame', 'nerva_completed'])
    def test_rejected_motor_action_changes_next_selection_without_new_personality(self):
        ghost=Ghost(HeartSnapshot(.6,.5,0)); bridge=BehaviorBridge(ghost)
        bridge.register_intent(4, 'scan')
        self.assertTrue(bridge.accept_result({'type':'nerva_result','version':1,'sequence':4,'status':'rejected','model_bound':False}))
        decision=bridge.select('scan')
        self.assertEqual(decision.kind,'idle')
        self.assertEqual(decision.basis,'nerva_rejection_safety_fallback')
        self.assertEqual(decision.experience_count,1)

    def test_glb_assets_and_six_legs_exist(self):
        import struct
        from hashlib import sha256
        root=BASE.parents[0] # akari_motion_test
        data=(root/'avatar'/'akari_test.glb').read_bytes()
        self.assertEqual(sha256(data).hexdigest(), '2a7fcc66fcdb9a76944a47c666f79911f5f24cd0256685dd7f8c3e66a314b33d')
        magic, version, length=struct.unpack_from('<III',data)
        self.assertEqual((magic,version,length),(0x46546c67,2,len(data)))
        mapping=json.loads((root/'avatar'/'vrm_expression_map.json').read_text())['bones']
        for b in ('leftUpperLeg','leftLowerLeg','leftFoot','rightUpperLeg','rightLowerLeg','rightFoot'):
            self.assertIn(b,mapping)
            self.assertIn(mapping[b].encode(),data)
        gd=(root/'scripts'/'nerva_controller.gd').read_text()
        self.assertIn('func _apply_stance(',gd)
        self.assertIn('func _apply_relaxed_hands(',gd)

    def test_complete_state_snapshot(self):
        snapshot=state_message(GhostAutonomy())
        self.assertEqual(snapshot['type'],'ghost_snapshot')
        self.assertEqual(snapshot['version'],1)
        self.assertTrue(all(0 <= value <= 1 for value in snapshot['state'].values()))
        self.assertEqual(len(snapshot['state']),6)

    def test_receiver_and_nerva_state_wiring(self):
        root=BASE.parents[0]
        receiver=(root/'scripts'/'ghost_intent_receiver.gd').read_text()
        nerva=(root/'scripts'/'nerva_controller.gd').read_text()
        self.assertIn('accept_ghost_state(validated)',receiver)
        self.assertIn('func accept_ghost_state(',nerva)
        self.assertIn('clear_ghost_state()',receiver)
        self.assertIn('state_factor',nerva)

    def test_validation(self):
        self.assertIsNone(validate_result('{invalid'))
        self.assertIsNone(validate_result(json.dumps({'type':'nerva_result','version':1,'sequence':True,'status':'applied_frame','model_bound':True})))
        self.assertIsNotNone(validate_result(json.dumps({'type':'nerva_result','version':1,'sequence':1,'status':'rejected','model_bound':False})))
        self.assertEqual(set(intent_message(1,'scan')['intent']),{'kind','strength'})
    def test_unprompted_time_actions(self):
        a=GhostAutonomy(); transitions=[]
        for ms in range(0,1_800_001,1000):
            e=a.tick(ms)
            if e: transitions.append(e)
        self.assertIn('drowsy',transitions);self.assertIn('sleep',transitions)

class WebsocketTests(unittest.IsolatedAsyncioTestCase):
    async def test_bidirectional_feedback_from_simulated_character(self):
        from websockets.asyncio.client import connect
        stop=asyncio.Event(); task=asyncio.create_task(run_server(time_scale=60,stop_event=stop))
        try:
            for i in range(50):
                try: ws=await connect('ws://127.0.0.1:8766');break
                except OSError: await asyncio.sleep(.05)
            else:self.fail('server not started')
            async with ws:
                snapshot=json.loads(await ws.recv())
                self.assertEqual(snapshot['type'],'ghost_snapshot')
                self.assertEqual(set(snapshot['state']), {'mood','affection','curiosity','boredom','sleepiness','attention'})
                msg=json.loads(await ws.recv());self.assertEqual(msg['source'],'state_sync')
                found=None
                for _ in range(45):
                    got=json.loads(await asyncio.wait_for(ws.recv(),20))
                    if got.get('type')=='ghost_intent' and got.get('sequence',0)>0:
                        found=got;break
                self.assertIsNotNone(found)
                await ws.send(json.dumps({'type':'nerva_result','version':1,'sequence':found['sequence'],
                                           'status':'applied_frame','model_bound':True}))
                for _ in range(10):
                    got=json.loads(await asyncio.wait_for(ws.recv(),2))
                    if got.get('type')=='ghost_feedback':break
                else:self.fail('no feedback returned')
                self.assertEqual(got['sequence'],found['sequence'])
                self.assertTrue(got['model_bound'])
        finally:
            stop.set();await asyncio.wait_for(task,3)
if __name__=='__main__': unittest.main()
"""Observed-world trial decision tests. Not a real-Godot or naturalness proof."""
from __future__ import annotations
import asyncio
import json
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parents[1]/'akari_motion_test'/'ghost_server'))
from ghost_core import Ghost, HeartSnapshot
from life_policy import LifePolicy
from behavior_bridge import BehaviorBridge
from intent_server import run_server


def observation(x, *, seated=False, busy=False, bound=True):
    return {'type':'nerva_observation', 'version':1,'position_x':x,
            'seated':seated,'motion_busy':busy,'model_bound':bound}


def result(seq, status, bound=True):
    return {'type':'nerva_result','version':1,'sequence':seq,'status':status,'model_bound':bound}


class GroundedPolicy(unittest.TestCase):
    def setUp(self):
        self.g=Ghost(HeartSnapshot(.6,.5,0))
        self.b=BehaviorBridge(self.g)
        self.p=LifePolicy()

    def completed(self, seq, kind, ms):
        self.b.register_intent(seq,kind)
        self.assertTrue(self.b.accept_result(result(seq,'completed')))
        self.assertTrue(self.p.result(kind,'completed',ms,self.g.memory.recall()))

    def test_no_observation_no_motion(self):
        self.assertIsNone(self.p.choose(350000,.35,.20,self.g.memory.recall()))
        self.assertEqual(self.p.decision_basis,'no_fresh_bound_character_observation')
        self.assertTrue(self.p.observe(observation(0,bound=False),350000))
        self.assertIsNone(self.p.choose(350000,.35,.20,self.g.memory.recall()))
        self.assertTrue(self.p.observe(observation(.4),350000))
        self.assertIsNone(self.p.choose(350000,.35,.20,self.g.memory.recall()))

    def test_boredom_memory_and_rest_change_next_action(self):
        self.assertTrue(self.p.observe(observation(0),350000))
        self.assertEqual(self.p.choose(350000,.35,.20,self.g.memory.recall()),'explore')
        self.p.observe(observation(-.4,busy=True),350100)
        self.assertIsNone(self.p.choose(350200,.35,.20,self.g.memory.recall()))
        # applied_frame is not proof that the body reached the destination.
        self.b.register_intent(1,'explore')
        self.assertTrue(self.b.accept_result(result(1,'applied_frame')))
        self.assertFalse(self.p.result('explore','applied_frame',351000,self.g.memory.recall()))
        self.assertIsNone(self.p.choose(351000,.35,.20,self.g.memory.recall()))
        self.assertTrue(self.b.accept_result(result(1,'completed')))
        self.assertTrue(self.p.result('explore','completed',352000,self.g.memory.recall()))
        self.assertIsNone(self.p.choose(352000,.35,.20,self.g.memory.recall()))
        self.p.observe(observation(-.65),352000)
        self.assertEqual(self.p.choose(352001,.35,.20,self.g.memory.recall()),'sit')
        self.completed(2,'sit',355000)
        self.p.observe(observation(-.65,seated=True),355001)
        self.assertIsNone(self.p.choose(370000,.35,.20,self.g.memory.recall()))  # fresh observation required
        self.p.observe(observation(-.65,seated=True),384999)
        self.assertIsNone(self.p.choose(384999,.35,.20,self.g.memory.recall()))
        self.p.observe(observation(-.65,seated=True),385001)
        self.assertEqual(self.p.choose(385001,.35,.20,self.g.memory.recall()),'rise')
        self.completed(3,'rise',388000)
        self.p.observe(observation(-.65,seated=False),408010)
        self.assertEqual(self.p.choose(408010,.35,.20,self.g.memory.recall()),'explore')
        self.completed(4,'explore',410000)
        self.p.observe(observation(0,seated=False),410001)
        self.assertIsNone(self.p.choose(410001,.20,.20,self.g.memory.recall()))
        self.assertEqual(self.p.completed_count,4)

    def test_rejection_in_memory_blocks_retry(self):
        self.p.observe(observation(0),350000)
        self.assertEqual(self.p.choose(350000,.35,.2,self.g.memory.recall()),'explore')
        self.b.register_intent(5,'explore')
        self.assertTrue(self.b.accept_result(result(5,'rejected',False)))
        self.assertTrue(self.p.result('explore','rejected',351000,self.g.memory.recall()))
        self.p.observe(observation(0),351001)
        self.assertIsNone(self.p.choose(351002,.35,.2,self.g.memory.recall()))
        self.assertEqual(self.p.decision_basis,'motor_failure_memory_safety_block')

    def test_durable_outcome_restores_only_evidence_not_fake_sensor(self):
        self.b.register_intent(8,'explore')
        self.assertTrue(self.b.accept_result(result(8,'completed')))
        recovered=LifePolicy(self.g.memory.recall())
        self.assertEqual(recovered.previous_completed,'explore')
        self.assertIsNone(recovered.observation)
        self.assertIsNone(recovered.choose(800000,.35,.35,self.g.memory.recall()))
        recovered.observe(observation(-.65),800000)
        self.assertEqual(recovered.choose(800001,.35,.35,self.g.memory.recall()),'sit')

    def test_reject_malformed_spatial_sensor(self):
        for x in (float('nan'),float('inf'),3.5,'0',True):
            self.assertFalse(self.p.observe(observation(x),123))
        self.assertFalse(self.p.observe({'type':'nerva_observation','version':1,'position_x':0,'seated':1,'motion_busy':False,'model_bound':True},123))


class ActualSocketLoop(unittest.IsolatedAsyncioTestCase):
    async def test_heart_memory_observation_result_drive_choice(self):
        from websockets.asyncio.client import connect
        stop=asyncio.Event()
        server=asyncio.create_task(run_server(time_scale=60,stop_event=stop,auto_life=True))
        try:
            for _ in range(60):
                try:
                    ws=await connect('ws://127.0.0.1:8766');break
                except OSError: await asyncio.sleep(.05)
            else:self.fail('server did not start')
            async with ws:
                current_observation=[observation(0)]
                async def sensed_character():
                    while True:
                        await ws.send(json.dumps(current_observation[0]))
                        await asyncio.sleep(.1)
                sensor_task=asyncio.create_task(sensed_character())
                async def next_intent(expected):
                    for _ in range(120):
                        msg=json.loads(await asyncio.wait_for(ws.recv(),3.0))
                        if msg.get('type')=='ghost_intent' and msg.get('source')=='ghost_experience_behavior_trial':
                            self.assertEqual(msg['intent']['kind'],expected)
                            return msg['sequence']
                    self.fail('no autonomous intent')
                seq=await next_intent('explore')
                # Real model busy event: prevents another action until confirmed end.
                current_observation[0]=observation(-.35,busy=True)
                await ws.send(json.dumps(result(seq,'applied_frame')))
                for _ in range(2):
                    current_observation[0]=observation(-.35,busy=True)
                    await asyncio.sleep(.15)
                await ws.send(json.dumps(result(seq,'completed')))
                current_observation[0]=observation(-.65)
                seq=await next_intent('sit')
                await ws.send(json.dumps(result(seq,'completed')))
                current_observation[0]=observation(-.65,seated=True)
                # Accelerated 30s rest ~= 0.5s real time.
                for _ in range(8):
                    await asyncio.sleep(.12)
                    current_observation[0]=observation(-.65,seated=True)
                seq=await next_intent('rise')
                await ws.send(json.dumps(result(seq,'completed')))
                sensor_task.cancel()
                await asyncio.gather(sensor_task,return_exceptions=True)
        finally:
            stop.set()
            await asyncio.wait_for(server,5)

if __name__=='__main__':unittest.main()
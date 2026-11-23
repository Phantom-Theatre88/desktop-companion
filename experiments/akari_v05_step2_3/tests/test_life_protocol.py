"""Regression for actual completion, not merely accepted frame or a timer."""
import asyncio
import json
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parents[1] / 'akari_motion_test' / 'ghost_server'))
from ghost_auto import GhostAutonomy
from ghost_core import Ghost, HeartSnapshot
from behavior_bridge import BehaviorBridge
from intent_server import run_server


def report(seq, status, bound=True):
    return {'type':'nerva_result', 'version':1, 'sequence':seq, 'status':status, 'model_bound':bound}


class ProtocolTests(unittest.TestCase):
    def test_progress_then_terminal_is_two_events(self):
        g=Ghost(HeartSnapshot(.6,.5,0)); bridge=BehaviorBridge(g)
        bridge.register_intent(3,'explore')
        self.assertTrue(bridge.accept_result(report(3,'applied_frame')))
        self.assertIn(3, bridge.pending)
        self.assertNotIn(3, bridge.results)
        self.assertFalse(bridge.accept_result(report(3,'applied_frame')))
        self.assertTrue(bridge.accept_result(report(3,'completed')))
        self.assertNotIn(3, bridge.pending)
        self.assertEqual([x.event.kind for x in g.memory.recall()], ['nerva_applied_frame','nerva_completed'])
        self.assertFalse(bridge.accept_result(report(3,'completed')))
        self.assertFalse(bridge.accept_result(report(3,'rejected',False)))

    def test_invalid_success_cannot_advance_trial(self):
        g=Ghost(HeartSnapshot(.6,.5,0)); bridge=BehaviorBridge(g)
        a=GhostAutonomy(life_trial_enabled=True);a.tick(15000)
        bridge.register_intent(7,'explore')
        self.assertFalse(bridge.accept_result(report(7,'completed',False)))
        self.assertIsNone(a.tick(30000))
        self.assertTrue(a.awaiting_result)

    def test_receiver_accepts_all_trial_kinds(self):
        text=(Path(__file__).parents[1]/'akari_motion_test'/'scripts'/'ghost_intent_receiver.gd').read_text()
        self.assertIn('"explore", "sit", "rise"',text)
        self.assertIn('_send_motion_result_if_ready()',text)


class RealWebsocketProtocol(unittest.IsolatedAsyncioTestCase):
    async def test_actual_completed_feedback_advances_all_three_stages(self):
        from websockets.asyncio.client import connect
        stop=asyncio.Event()
        server=asyncio.create_task(run_server(time_scale=60,stop_event=stop,life_trial=True))
        try:
            for _ in range(50):
                try:
                    ws=await connect('ws://127.0.0.1:8766');break
                except OSError:await asyncio.sleep(.05)
            else:self.fail('server did not start')
            async with ws:
                async def next_intent():
                    for _ in range(200):
                        item=json.loads(await asyncio.wait_for(ws.recv(),3))
                        if item.get('type')=='ghost_intent' and item.get('source')!='state_sync':return item
                    self.fail('no trial intent')
                for expected in ('explore','sit','rise'):
                    msg=await next_intent()
                    self.assertEqual(msg['intent']['kind'],expected)
                    seq=msg['sequence']
                    await ws.send(json.dumps(report(seq,'applied_frame')))
                    # No next action should be emitted merely on applied_frame.
                    for _ in range(5):
                        try:item=json.loads(await asyncio.wait_for(ws.recv(),0.15))
                        except asyncio.TimeoutError:break
                        if item.get('type')=='ghost_intent' and item.get('source')!='state_sync':
                            self.fail('next action emitted before completed')
                    await ws.send(json.dumps(report(seq,'completed')))
                last=await next_intent()
                self.assertEqual(last['intent']['kind'],'idle')
        finally:
            stop.set()
            await asyncio.wait_for(server,5)


if __name__=='__main__':unittest.main()
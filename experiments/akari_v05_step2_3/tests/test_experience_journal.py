"""Test durable experience facts independently of unapproved personality rules."""
import asyncio
import json
import sqlite3
import sys
import tempfile
import unittest
from pathlib import Path
BASE = Path(__file__).resolve().parents[1] / 'akari_motion_test' / 'ghost_server'
sys.path.insert(0, str(BASE))
from behavior_bridge import BehaviorBridge
from experience_journal import ExperienceJournal
from ghost_core import Ghost, HeartSnapshot, SemanticEvent, Experience


class JournalTests(unittest.TestCase):
    def test_restart_keeps_fact_and_historical_heart_not_current_feelings(self):
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / 'state.sqlite'
            initial = HeartSnapshot(0.6, 0.5, 0)
            j = ExperienceJournal(path)
            original = Ghost(initial)
            bridge = BehaviorBridge(original, j)
            bridge.register_intent(1, 'scan')
            self.assertTrue(bridge.accept_result({'type': 'nerva_result', 'version': 1,
                                                  'sequence': 1, 'status': 'rejected',
                                                  'model_bound': False}))
            exp = original.memory.recall()[0]
            self.assertIsNone(exp.event.actor_id)
            j.close()
            j2 = ExperienceJournal(path)
            resumed = Ghost(initial)
            resumed_bridge = BehaviorBridge(resumed, j2)
            self.assertEqual(resumed.memory.recall(), (exp,))
            self.assertEqual(resumed.heart.snapshot(), initial)
            self.assertEqual(resumed_bridge.select('scan').experience_count, 1)
            self.assertEqual(resumed_bridge.select('scan').relationship_count, 0)
            resumed_bridge.register_intent(1, 'blink')
            self.assertTrue(resumed_bridge.accept_result({'type':'nerva_result','version':1,
                                        'sequence':1,'status':'applied_frame','model_bound':True}))
            self.assertEqual(len(j2.read_all()), 2)
            self.assertEqual(len(set(e.event.event_id for e in j2.read_all())), 2)
            j2.close()

    def test_actor_history_is_not_invented_or_mutated(self):
        with tempfile.TemporaryDirectory() as td:
            j = ExperienceJournal(Path(td)/'state.sqlite')
            before = HeartSnapshot(.33, .64, 4)
            after = HeartSnapshot(.36, .6, 7)
            exp = Experience(SemanticEvent('touch-explicit','touch',7,'observed_actor'),before,after,'reason_unknown')
            j.append(exp)
            self.assertEqual(j.read_all(), (exp,))
            g = Ghost(HeartSnapshot(.6,.5,0)); b = BehaviorBridge(g,j)
            self.assertEqual(b.select('idle','observed_actor').relationship_count,1)
            self.assertEqual(b.select('idle','other_actor').relationship_count,0)
            self.assertEqual(g.memory.recall()[0].heart_before,before)
            with self.assertRaises(sqlite3.IntegrityError):j.append(exp)
            j.close()

    def test_failure_to_save_must_not_acknowledge(self):
        class FailingJournal:
            def read_all(self):return ()
            def append(self,event):raise sqlite3.OperationalError('disk unavailable')
        g=Ghost(HeartSnapshot(.6,.5,0));b=BehaviorBridge(g,FailingJournal())
        b.register_intent(5,'scan')
        with self.assertRaises(sqlite3.OperationalError):
            b.accept_result({'type':'nerva_result','version':1,'sequence':5,
                            'status':'rejected','model_bound':False})
        self.assertEqual(g.memory.recall(),())
        self.assertIn(5,b.pending)
        self.assertNotIn(5,b.results)

    def test_no_database_created_without_opt_in(self):
        with tempfile.TemporaryDirectory() as td:
            g=Ghost(HeartSnapshot(.6,.5,0));b=BehaviorBridge(g)
            b.register_intent(8,'scan')
            self.assertTrue(b.accept_result({'type':'nerva_result','version':1,
                                            'sequence':8,'status':'rejected','model_bound':False}))
            self.assertFalse(list(Path(td).iterdir()))

class ServerRestartTests(unittest.IsolatedAsyncioTestCase):
    async def test_wire_result_survives_server_restart_without_new_personality(self):
        from websockets.asyncio.client import connect
        from intent_server import run_server
        with tempfile.TemporaryDirectory() as td:
            path = Path(td)/'ledger.sqlite'
            for run_number in range(2):
                stop=asyncio.Event()
                task=asyncio.create_task(run_server(time_scale=60,stop_event=stop,
                                                    journal_path=path))
                try:
                    for _ in range(70):
                        try:
                            ws=await connect('ws://127.0.0.1:8766')
                            break
                        except OSError:
                            await asyncio.sleep(.03)
                    else:
                        self.fail('server not started')
                    async with ws:
                        await ws.recv()  # initial 6-state snapshot
                        await ws.recv()  # reconnect state-sync intent (not autonomous success)
                        for _ in range(80):
                            packet=json.loads(await asyncio.wait_for(ws.recv(),6))
                            if packet.get('type')=='ghost_intent' and packet.get('sequence',0)>0:
                                break
                        else:
                            self.fail('no autonomous intent')
                        await ws.send(json.dumps({'type':'nerva_result','version':1,
                            'sequence':packet['sequence'],'status':'applied_frame',
                            'model_bound':True}))
                        for _ in range(20):
                            feedback=json.loads(await asyncio.wait_for(ws.recv(),3))
                            if feedback.get('type')=='ghost_feedback':break
                        else:self.fail('no server feedback')
                        self.assertTrue(feedback['model_bound'])
                finally:
                    stop.set()
                    await asyncio.wait_for(task,4)
            j=ExperienceJournal(path)
            self.assertEqual(len(j.read_all()),2)
            self.assertEqual(len(set(e.event.event_id for e in j.read_all())),2)
            self.assertTrue(all(e.event.actor_id is None for e in j.read_all()))
            j.close()

if __name__ == '__main__': unittest.main()
"""AKARI local WebSocket integration prototype; not production personality.
Ghost's state/selection stays independent from diagnostic button commands.
Character reports actual bound-frame application, not subjective motion quality.
"""
from __future__ import annotations
import asyncio
import argparse
import json
import math
import sys
import time
from pathlib import Path
from websockets.asyncio.server import serve
from websockets.exceptions import ConnectionClosed
from ghost_core import Ghost, HeartSnapshot, SemanticEvent
from ghost_auto import GhostAutonomy
from behavior_bridge import BehaviorBridge
from experience_journal import ExperienceJournal
from life_policy import LifePolicy

ALLOWED = frozenset({'near','quiet','blink','status'})
TEST_INTENTS = {'near': 'attend', 'quiet':'rest','blink':'blink'}
HEART_KEYS = ('mood','affection','curiosity','boredom','sleepiness','attention')

def state_message(autonomy: GhostAutonomy) -> dict:
    return {'type':'ghost_snapshot','version':1,'state':{k:getattr(autonomy,k) for k in HEART_KEYS},
            'source':'ghost_time_compatibility'}


def intent_message(seq: int, kind: str, source='ghost_time_compatibility') -> dict:
    return {'type':'ghost_intent','version':1,'sequence':seq,
            'intent':{'kind':kind,'strength':0.6},'source':source}

def validate_result(text: str) -> dict | None:
    if len(text)>1024: return None
    try: data=json.loads(text)
    except (ValueError,TypeError): return None
    if not isinstance(data,dict): return None
    if data.get('type')!='nerva_result' or data.get('version')!=1: return None
    if type(data.get('sequence')) is not int: return None
    if data.get('status') not in ('rejected','applied_frame','completed','interrupted'): return None
    if type(data.get('model_bound')) is not bool: return None
    return data

async def run_server(time_scale=1.0,stop_event=None,headless=True,journal_path=None,life_trial=False,auto_life=False):
    if not 1<=time_scale<=60: raise ValueError('time_scale range 1..60')
    ghost=Ghost(HeartSnapshot(.60,.5,0))
    autonomy=GhostAutonomy(life_trial_enabled=life_trial)
    journal = ExperienceJournal(journal_path) if journal_path is not None else None
    bridge=BehaviorBridge(ghost, journal)
    policy = LifePolicy(ghost.memory.recall()) if auto_life else None
    clients=set(); sequence=0; started=time.monotonic(); lock=asyncio.Lock()
    last_snapshot_second = -1
    async def send(payload):
        message=json.dumps(payload,ensure_ascii=False)
        for ws in tuple(clients):
            try: await ws.send(message)
            except ConnectionClosed: clients.discard(ws)
        print(message,flush=True)
    async def emit(kind,source,decision_basis=None):
        nonlocal sequence
        selection=bridge.select(kind)
        sequence+=1
        bridge.register_intent(sequence, selection.kind)
        await send(intent_message(sequence,selection.kind,source))
        await send({'type':'behavior_evidence','version':1,'sequence':sequence,
                    'experience_count':selection.experience_count,
                    'relationship_count':selection.relationship_count,
                    'last_result':selection.last_result,'basis':decision_basis or selection.basis,
                    'heart_compatibility':state_message(autonomy)['state']})
    async def tick_loop():
        nonlocal last_snapshot_second
        while True:
            await asyncio.sleep(0.2)
            async with lock:
                elapsed_ms=int((time.monotonic()-started)*1000*time_scale)
                baseline_kind=autonomy.tick(elapsed_ms)
                if policy is not None:
                    kind = policy.choose(elapsed_ms, autonomy.boredom, autonomy.sleepiness, ghost.memory.recall(),
                                         observation_now_ms=int((time.monotonic()-started)*1000))
                    if kind is not None: await emit(kind,'ghost_experience_behavior_trial',policy.decision_basis)
                elif baseline_kind is not None:
                    await emit(baseline_kind,'ghost_time_compatibility')
                # Keep Character informed even when the discrete intent is unchanged.
                # A state snapshot carries measured/derived values, never new emotions.
                snapshot_second = elapsed_ms // 1000
                if snapshot_second != last_snapshot_second:
                    last_snapshot_second = snapshot_second
                    await send(state_message(autonomy))
    async def connection(ws):
        clients.add(ws)
        async with lock:
            await ws.send(json.dumps(state_message(autonomy)))
            await ws.send(json.dumps(intent_message(sequence,autonomy.action,'state_sync')))
        try:
            async for raw in ws:
                try: inbound = json.loads(raw)
                except (ValueError, TypeError): continue
                if policy is not None and isinstance(inbound, dict) and inbound.get('type') == 'nerva_observation':
                    async with lock:
                        now_ms=int((time.monotonic()-started)*1000*time_scale)
                        policy.observe(inbound, int((time.monotonic()-started)*1000))
                    continue
                data=validate_result(raw)
                if data is None: continue
                async with lock:
                    if bridge.accept_result(data):
                        if policy is not None:
                            now_ms=int((time.monotonic()-started)*1000*time_scale)
                            policy.result(bridge.intent_kinds.get(data['sequence'],''), data['status'], now_ms, ghost.memory.recall())
                        autonomy.on_motion_result(bridge.intent_kinds.get(data['sequence'], ''), data['status'])
                        await send({'type':'ghost_feedback','version':1,'sequence':data['sequence'],
                                    'status':data['status'],'model_bound':data['model_bound']})
                        # A failed, non-idle action must not be endlessly retried.
                        # Emit safe idle once; this is transport safety, NOT personality.
                        if data['status']=='rejected' and bridge.intent_kinds.get(data['sequence']) != 'idle':
                            if bridge.select(autonomy.action).basis == 'nerva_rejection_safety_fallback':
                                await emit(autonomy.action,'nerva_rejection_safety_fallback')
        except ConnectionClosed: pass
        finally: clients.discard(ws)
    async with serve(connection,'127.0.0.1',8766):
        print('AKARI Ghost integration: ws://127.0.0.1:8766',flush=True)
        task=asyncio.create_task(tick_loop())
        try:
            if stop_event is not None: await stop_event.wait()
            elif headless: await asyncio.Future()
            else:
                while True:
                    line=await asyncio.to_thread(sys.stdin.readline)
                    kind=line.strip()
                    if not kind or kind=='quit': break
                    if kind=='status': print(json.dumps({'candidate':autonomy.action,'results':bridge.results,'experiences':len(ghost.memory.recall()),'life_phase':autonomy.life_trial_phase,'motion_completions':autonomy.trial_completions}),flush=True)
                    elif kind in TEST_INTENTS:
                        # Explicit human diagnostic input is not autonomous experience.
                        async with lock: await emit(TEST_INTENTS[kind],'console_test')
        finally:
            task.cancel(); await asyncio.gather(task,return_exceptions=True)
            if journal is not None: journal.close()

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--time-scale',type=float,default=1.0)
    parser.add_argument('--headless',action='store_true')
    parser.add_argument('--life-trial', action='store_true', help='Original scripted regression trial')
    parser.add_argument('--auto-life', action='store_true', help='Provisional observed-stage Ghost behavior; not a personality LOCK')
    parser.add_argument('--journal',type=Path,default=None,help='Optional experimental durable Memory ledger (SQLite)')
    args=parser.parse_args()
    asyncio.run(run_server(args.time_scale,headless=args.headless,journal_path=args.journal,life_trial=args.life_trial,auto_life=args.auto_life))
if __name__=='__main__':main()
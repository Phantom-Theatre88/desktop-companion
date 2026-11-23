"""AKARI local WebSocket integration prototype; not production personality.
Ghost's state/selection stays independent from diagnostic button commands.
Character reports actual bound-frame application, not subjective motion quality.
"""
from __future__ import annotations
import asyncio
import argparse
import json
import sys
import time
from pathlib import Path
from websockets.asyncio.server import serve
from websockets.exceptions import ConnectionClosed
from ghost_core import Ghost, HeartSnapshot, SemanticEvent
from ghost_auto import GhostAutonomy
from behavior_bridge import BehaviorBridge

ALLOWED = frozenset({'near','quiet','blink','status'})
TEST_INTENTS = {'near': 'attend', 'quiet':'rest','blink':'blink'}

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

async def run_server(time_scale=1.0,stop_event=None,headless=True):
    if not 1<=time_scale<=60: raise ValueError('time_scale range 1..60')
    ghost=Ghost(HeartSnapshot(.60,.5,0))
    autonomy=GhostAutonomy()
    bridge=BehaviorBridge(ghost)
    clients=set(); sequence=0; started=time.monotonic(); lock=asyncio.Lock()
    async def send(payload):
        message=json.dumps(payload,ensure_ascii=False)
        for ws in tuple(clients):
            try: await ws.send(message)
            except ConnectionClosed: clients.discard(ws)
        print(message,flush=True)
    async def emit(kind,source):
        nonlocal sequence
        selection=bridge.select(kind)
        sequence+=1
        bridge.register_intent(sequence, selection.kind)
        await send(intent_message(sequence,selection.kind,source))
        await send({'type':'behavior_evidence','version':1,'sequence':sequence,
                    'experience_count':selection.experience_count,
                    'relationship_count':selection.relationship_count,
                    'last_result':selection.last_result,'basis':selection.basis,
                    'heart_compatibility':{k:getattr(autonomy,k) for k in
                    ('mood','affection','curiosity','boredom','sleepiness','attention')}})
    async def tick_loop():
        while True:
            await asyncio.sleep(0.2)
            async with lock:
                elapsed_ms=int((time.monotonic()-started)*1000*time_scale)
                kind=autonomy.tick(elapsed_ms)
                if kind is not None: await emit(kind,'ghost_time_compatibility')
    async def connection(ws):
        clients.add(ws)
        async with lock: await ws.send(json.dumps(intent_message(sequence,autonomy.action,'state_sync')))
        try:
            async for raw in ws:
                data=validate_result(raw)
                if data is None: continue
                async with lock:
                    if bridge.accept_result(data):
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
                    if kind=='status': print(json.dumps({'candidate':autonomy.action,'results':bridge.results,'experiences':len(ghost.memory.recall())}),flush=True)
                    elif kind in TEST_INTENTS:
                        # Explicit human diagnostic input is not autonomous experience.
                        async with lock: await emit(TEST_INTENTS[kind],'console_test')
        finally:
            task.cancel(); await asyncio.gather(task,return_exceptions=True)

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--time-scale',type=float,default=1.0)
    parser.add_argument('--headless',action='store_true')
    args=parser.parse_args()
    asyncio.run(run_server(args.time_scale,headless=args.headless))
if __name__=='__main__':main()
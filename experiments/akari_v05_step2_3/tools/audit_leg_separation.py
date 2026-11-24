#!/usr/bin/env python3
"""Reject seated ankle crossing and jerky motor correction in real AKARI GLB.

Checks actual GLB joint-origin FK, not animation names or channel count.
Walking is exempt: a normal swing leg can pass across the body during gait.
"""
from __future__ import annotations
import json,math,sys
from pathlib import Path
from install_cc0_motion import glb_read,read_accessor,quat_mul,quat_inv
from audit_mocap_pose import sample_pose

MIN_SEATED_TOE_GAP = 0.10
CLIPS=('sit','seated_idle','rise')

def audit_leg_separation_bytes(payload,bones):
    js,blob=glb_read(payload)
    names={n.get('name'):i for i,n in enumerate(js['nodes'])}
    result={}
    for name in CLIPS:
        anim=next((a for a in js.get('animations',()) if a.get('name')==name),None)
        if anim is None:raise ValueError('Missing seated animation '+name)
        nodename=bones['rightUpperLeg'];node=names[nodename]
        track=next((c for c in anim['channels'] if c['target']=={'node':node,'path':'rotation'}),None)
        if track is None:raise ValueError('Missing actual right leg rotation track in '+name)
        times=read_accessor(js,blob,anim['samplers'][track['sampler']]['input'])
        sampled=max(1,len(times)-1)
        gaps=[];ymin=100
        for k in range(len(times)):
            pose=sample_pose(js,blob,bones,name,k/sampled)
            gap=pose['leftToes'][0]-pose['rightToes'][0]
            if not math.isfinite(gap):raise ValueError('Nonfinite leg separation in '+name)
            ymin=min(ymin,pose['rightToes'][1],pose['leftToes'][1])
            gaps.append(gap)
        minimum=min(gaps)
        if minimum < MIN_SEATED_TOE_GAP:
            raise ValueError('Seated toe crossing/too-close posture in '+name+': '+str(round(minimum,4))+'m')
        if ymin < -0.002:
            raise ValueError('Toe below floor in '+name+': '+str(ymin))
        result[name]={'min_left_minus_right_toe_x_m':round(minimum,4),'min_toe_height_m':round(ymin,4),'samples':len(times)}
    return result

if __name__=='__main__':
    root=Path(__file__).resolve().parents[1]
    bones=json.loads((root/'avatar/vrm_expression_map.json').read_text())['bones']
    payload=(Path(sys.argv[1]) if len(sys.argv)>1 else root/'avatar/akari_animated.glb').read_bytes()
    print(json.dumps(audit_leg_separation_bytes(payload,bones),indent=2))
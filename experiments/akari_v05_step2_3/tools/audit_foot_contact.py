#!/usr/bin/env python3
"""Ground clearance audit of actual AKARI retargeted GLB via forward kinematics.
Toe joint Y 0.04 m corresponds to the current sneaker sole resting on trial floor.
A hard floor penetration is rejected; this is not a complete foot-lock/IK runtime.
"""
import json, math, sys
from pathlib import Path
from install_cc0_motion import glb_read
from audit_mocap_pose import sample_pose
REQUIRED=('idle','walk','sit','seated_idle','rise')
SAMPLES=[i/40 for i in range(41)]

def audit_foot_contact_bytes(payload,bones):
    js,blob=glb_read(payload)
    result={}
    for clip in REQUIRED:
        min_toe=100.0
        for frac in SAMPLES:
            pose=sample_pose(js,blob,bones,clip,frac)
            for side in ('left','right'):
                toe=pose[side+'Toes']
                min_toe=min(min_toe,toe[1])
                if not all(math.isfinite(v) for v in toe):
                    raise ValueError('Nonfinite foot in '+clip)
        result[clip]={'min_toe_y_m':round(min_toe,4)}
        if min_toe < -0.002:
            raise ValueError(f'Foot below trial floor in {clip}: {min_toe:+.3f}m')
    return result

def main():
    root=Path(__file__).resolve().parents[1]
    bones=json.loads((root/'avatar/vrm_expression_map.json').read_text())['bones']
    payload=(Path(sys.argv[1]) if len(sys.argv)>1 else root/'avatar/akari_animated.glb').read_bytes()
    print(json.dumps(audit_foot_contact_bytes(payload,bones),ensure_ascii=False,indent=2))
if __name__=='__main__':main()
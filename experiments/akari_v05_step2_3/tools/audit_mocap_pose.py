#!/usr/bin/env python3
"""Validate actual humanoid wrist/shoulder geometry in generated GLB animations.

A valid clip name and 49 bone rotation tracks do not prove a usable pose.
This fails closed if ordinary walk/idle/sit/rise put both wrists above shoulders.
The check operates on final imported GLB, NOT on source file names.
"""
from __future__ import annotations
import json, math, sys
from pathlib import Path
from install_cc0_motion import glb_read, read_accessor, quat_mul, quat_rotate, quat_norm

CORE = ('idle', 'walk', 'sit', 'seated_idle', 'rise')
SAMPLES = (0., 0.25, 0.5, 0.75, 0.99)


def sample_pose(js, blob: bytes, bones: dict, animation_name: str, at_fraction: float):
    """Evaluate world-space joint origins using actual GLTF animation keys."""
    nodes = js['nodes']
    names = {n.get('name'): i for i, n in enumerate(nodes)}
    anim = next((a for a in js.get('animations', ()) if a.get('name') == animation_name), None)
    if anim is None:
        raise ValueError('Animation not present: ' + animation_name)
    parents = {child: i for i, n in enumerate(nodes) for child in n.get('children', ())}
    tracks = {}
    length = 0.0
    for channel in anim['channels']:
        t = channel['target']
        if t.get('path') not in ('rotation', 'translation'):
            continue
        sampler = anim['samplers'][channel['sampler']]
        times = [x[0] for x in read_accessor(js, blob, sampler['input'])]
        values = read_accessor(js, blob, sampler['output'])
        if not times or not values:
            continue
        length = max(length, times[-1])
        tracks[(t['node'], t['path'])] = (times, values)
    clock = length * at_fraction

    def interpolate(times, values, path):
        if clock <= times[0]:
            return values[0]
        if clock >= times[-1]:
            return values[-1]
        import bisect
        idx = bisect.bisect_right(times, clock) - 1
        idx = min(idx, len(times)-2)
        duration = times[idx+1] - times[idx]
        alpha = 0.0 if duration <= 0 else (clock - times[idx]) / duration
        v0, v1 = values[idx], values[idx+1]
        if path == 'rotation':
            if sum(a*b for a,b in zip(v0,v1)) < 0:
                v1 = [-x for x in v1]
            return quat_norm([a+(b-a)*alpha for a,b in zip(v0,v1)])
        return [a+(b-a)*alpha for a,b in zip(v0,v1)]

    cache = {}
    def transform(i):
        if i in cache: return cache[i]
        node = nodes[i]
        pos = node.get('translation', [0., 0., 0.])
        rot = node.get('rotation', [0., 0., 0., 1.])
        for path in ('translation', 'rotation'):
            if (i, path) in tracks:
                times, vals = tracks[(i,path)]
                val = interpolate(times, vals, path)
                if path == 'rotation': rot = val
                else: pos = val
        if i in parents:
            parent_pos, parent_rot = transform(parents[i])
            rotated = quat_rotate(parent_rot, pos)
            pos = [a+b for a,b in zip(parent_pos, rotated)]
            rot = quat_norm(quat_mul(parent_rot, rot))
        cache[i] = (pos, rot)
        return cache[i]

    result = {}
    for k in ('leftShoulder', 'leftHand', 'rightShoulder', 'rightHand', 'head', 'hips'):
        n = bones.get(k)
        if n not in names: raise ValueError('Missing bone ' + k + ':' + str(n))
        result[k] = transform(names[n])[0]
    return result


def audit_pose_bytes(data: bytes, bones: dict, must_have=CORE):
    js, blob = glb_read(data)
    report = {}
    for clip in must_have:
        samples = []
        for fraction in SAMPLES:
            pose = sample_pose(js, blob, bones, clip, fraction)
            side_gap = [pose[side+'Hand'][1] - pose[side+'Shoulder'][1]
                        for side in ('left','right')]
            values = {'at': fraction, 'left_wrist_minus_shoulder_m': round(side_gap[0],4),
                      'right_wrist_minus_shoulder_m': round(side_gap[1],4),
                      'hip_y_m': round(pose['hips'][1],4)}
            if not all(math.isfinite(x) for x in side_gap):
                raise ValueError('NaN humanoid pose '+clip)
            # These particular everyday clips are known to be hands-down/low.
            # Do not impose this rule on gestures, dance, or reaching animations.
            if any(gap > 0.12 for gap in side_gap):
                raise ValueError('Abnormal hands-up posture in '+clip+' at '+str(fraction)+': '+str(values))
            samples.append(values)
        report[clip] = samples
    return report


def main():
    base = Path(__file__).resolve().parents[1]
    mapfile = json.loads((base/'avatar/vrm_expression_map.json').read_text())['bones']
    path = Path(sys.argv[1]) if len(sys.argv) > 1 else base/'avatar/akari_animated.glb'
    report = audit_pose_bytes(path.read_bytes(),mapfile)
    print('PASS: no abnormal hands-up in', len(report), 'real motion clips')
    print(json.dumps(report,indent=2))

if __name__=='__main__': main()
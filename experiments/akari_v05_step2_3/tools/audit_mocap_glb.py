#!/usr/bin/env python3
"""Read-only glTF/GLB motion intake audit; stdlib only. No claims of retargeting success."""
import json, pathlib, struct, sys

def audit(path):
    path=pathlib.Path(path)
    if path.suffix.lower() not in ('.glb','.gltf'):
        raise ValueError('Expected .glb or .gltf')
    if path.suffix.lower()=='.gltf':
        data=json.loads(path.read_text(encoding='utf-8'))
    else:
        with path.open('rb') as stream:
            magic,version,size=struct.unpack('<4sII',stream.read(12))
            if magic!=b'glTF' or version!=2 or size!=path.stat().st_size:
                raise ValueError('Bad glTF 2 GLB header/length')
            length,ctype=struct.unpack('<II',stream.read(8))
            if ctype!=0x4e4f534a or length>path.stat().st_size-20:
                raise ValueError('Missing JSON chunk')
            data=json.loads(stream.read(length))
    animations=data.get('animations',[])
    if not animations:
        raise ValueError('No glTF animations: not usable as a motion clip')
    nodes=data.get('nodes',[])
    skins=data.get('skins',[])
    joints=set(sum((s.get('joints',[]) for s in skins),[]))
    driven=set(ch.get('target',{}).get('node') for a in animations for ch in a.get('channels',[]))
    mapped=[nodes[i].get('name',f'node_{i}') for i in driven & joints if isinstance(i,int) and i<len(nodes)]
    if not mapped:
        raise ValueError('Animations do not drive skeleton joints')
    return {'file':str(path),'clips':[a.get('name',f'clip_{i}') for i,a in enumerate(animations)],'skins':len(skins),'animated_joint_count':len(mapped),'joint_examples':sorted(mapped)[:20],'retarget_status':'UNVERIFIED: use Godot SkeletonProfileHumanoid/BoneMap'}

if __name__=='__main__':
    try:
        print(json.dumps(audit(sys.argv[1]),ensure_ascii=False,indent=2))
    except (ValueError,OSError,IndexError) as err:
        print('MOCAP INTAKE FAIL:',err,file=sys.stderr)
        sys.exit(1)
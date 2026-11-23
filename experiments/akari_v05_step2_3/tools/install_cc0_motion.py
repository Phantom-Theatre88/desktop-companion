#!/usr/bin/env python3
"""AKARI experimental CC0 humanoid clip importer. Original VRM/GLB remains untouched.

Downloads the published CC0 35-clip KinemIQ bundle, maps animated local bone
rotations onto AKARI's VRM humanoid bone list, and builds a separate GLB.
This is an approximate rotation retarget, not IK/contact correction; reject
incompatible clips rather than silently invent a successful import.
"""
from __future__ import annotations
import argparse, copy, hashlib, json, math, re, struct, urllib.request, zipfile
from pathlib import Path

URL = 'https://huggingface.co/datasets/Kinemiq/free-game-animations/resolve/main/KinemIQ_Free_Animations_CC0_GLB.zip'
EXPECTED_CC0_SHA256 = '64ca5ab4a92c9da079c5e88ca509391b0a58a24ccf2a1307b16e2f0fc6b23ffc'  # published source file SHA256
ROOT = Path(__file__).resolve().parents[1]
BUNDLE = ROOT/'motions'/'source'/'KinemIQ_Free_Animations_CC0_GLB.zip'
TARGET = ROOT/'avatar'/'akari_test.glb'
OUTPUT = ROOT/'avatar'/'akari_animated.glb'
MAP = ROOT/'avatar'/'vrm_expression_map.json'


def glb_read(data: bytes):
    if data[:4] != b'glTF' or struct.unpack_from('<I',data,4)[0] != 2:
        raise ValueError('Not a GLB2 file')
    pos=12; js=None; binary=b''
    while pos+8 <= len(data):
        size,kind=struct.unpack_from('<II',data,pos);pos+=8
        chunk=data[pos:pos+size];pos+=size
        if kind==0x4E4F534A: js=json.loads(chunk)
        if kind==0x004E4942: binary=chunk
    if js is None: raise ValueError('GLB missing JSON')
    return js,binary


def glb_pack(js,bin_data):
    jb=json.dumps(js,ensure_ascii=False,separators=(',',':')).encode('utf8');jb+=b' '*((-len(jb))%4)
    bin_data+=b'\0'*((-len(bin_data))%4)
    total=12+8+len(jb)+8+len(bin_data)
    return struct.pack('<4sII',b'glTF',2,total)+struct.pack('<II',len(jb),0x4E4F534A)+jb+struct.pack('<II',len(bin_data),0x004E4942)+bin_data


def normalize(name):
    # Common Quaternius, Mixamo, Blender, VRM naming conventions
    s=re.sub(r'(?i)^.*[:|]','',name)
    s=re.sub(r'[^a-z0-9]','',s.lower())
    for prefix in ('mixamorig','bip001','armature','j_bip','jbip','def','org'):
        if s.startswith(prefix): s=s[len(prefix):]
    return s

ALI={
 'hips':['hips','pelvis','hip'], 'spine':['spine','spine1','spine01'],
 'chest':['chest','spine2','spine02'], 'upperChest':['upperchest','spine3','spine03'],
 'neck':['neck'], 'head':['head'],
}
for side,long,short in [('left','left','l'),('right','right','r')]:
    for k,labels in {
        'Shoulder':['shoulder','clavicle'], 'UpperArm':['upperarm','arm'],
        'LowerArm':['lowerarm','forearm'], 'Hand':['hand','wrist'],
        'UpperLeg':['upperleg','upleg','thigh'], 'LowerLeg':['lowerleg','leg','calf','shin'],
        'Foot':['foot','ankle'], 'Toes':['toes','toebase','toe']}.items():
        ALI[side+k]=list(dict.fromkeys([long+x for x in labels]+[x+short for x in labels]+[short+x for x in labels]+[x+long for x in labels]))


def quat_mul(a,b):
    x,y,z,w=a;X,Y,Z,W=b
    return [w*X+x*W+y*Z-z*Y,w*Y-x*Z+y*W+z*X,w*Z+x*Y-y*X+z*W,w*W-x*X-y*Y-z*Z]

def quat_inv(q):
    d=sum(x*x for x in q)
    return [-q[0]/d,-q[1]/d,-q[2]/d,q[3]/d] if d else [0,0,0,1]

def quat_norm(q):
    d=math.sqrt(sum(x*x for x in q))
    return [x/d for x in q] if d else [0,0,0,1]


def global_rest_rotations(nodes):
    """Rest-space orientation used for coordinate-correct world delta retargeting."""
    parents={child:i for i,n in enumerate(nodes) for child in n.get('children',[])}
    cache={}
    def walk(i,depth=0):
        if i in cache:return cache[i]
        if depth>len(nodes):raise ValueError('Skeleton hierarchy cycle')
        q=nodes[i].get('rotation',[0,0,0,1])
        if i in parents:q=quat_norm(quat_mul(walk(parents[i],depth+1),q))
        cache[i]=q
        return q
    return [walk(i) for i in range(len(nodes))],parents


def read_accessor(js,blob,idx):
    a=js['accessors'][idx]; bv=js['bufferViews'][a['bufferView']]
    typ=a['type'];k={'SCALAR':1,'VEC3':3,'VEC4':4}[typ]
    dtype=a['componentType']; fmt={5126:'f',5123:'H',5125:'I'}.get(dtype)
    if not fmt:raise ValueError('Unsupported accessor dtype')
    sz=struct.calcsize('<'+fmt)*k; stride=bv.get('byteStride',sz)
    at=bv.get('byteOffset',0)+a.get('byteOffset',0)
    return [list(struct.unpack_from('<'+fmt*k,blob,at+i*stride)) for i in range(a['count'])]


def choose_motion_name(raw):
    text=normalize(raw)
    if ('sitdown' in text or 'sittingdown' in text or 'standtosit' in text): return 'sit'
    if ('standup' in text or 'standfromsit' in text or 'sittostand' in text or 'getupfromsit' in text): return 'rise'
    if 'walk' in text and 'back' not in text and 'side' not in text: return 'walk'
    if 'idle' in text and 'sit' in text: return 'seated_idle'
    if 'idle' in text or 'standrelaxed' in text: return 'idle'
    if 'stretch' in text: return 'stretch'
    if 'turn' in text and ('left' in text or 'right' in text): return 'turn'
    return None


def collect_motion(source: bytes, source_label: str, humanoid: dict, target_nodes):
    js,blob=glb_read(source)
    if not js.get('animations'): return []
    available={normalize(n.get('name','')):i for i,n in enumerate(js['nodes'])}
    source_global,source_parents=global_rest_rotations(js['nodes'])
    mapping={}
    for key,actual in humanoid.items():
        if key not in ALI:continue
        choices=ALI[key]+[key]
        idx=next((available[normalize(n)] for n in choices if normalize(n) in available),None)
        if idx is not None and actual in target_nodes: mapping[idx]=target_nodes[actual]
    if len(mapping)<10: return []
    candidate=[]
    for anim in js['animations']:
        name=choose_motion_name(source_label+' '+anim.get('name',''))
        if not name:continue
        channels=[]
        for channel in anim['channels']:
            srcid=channel['target'].get('node')
            if srcid not in mapping:continue
            path=channel['target'].get('path')
            # Hip translation is essential for sit/stand; other node translations
            # include root motion which NERVA already handles. Do not double move.
            if path != 'rotation' and not (path == 'translation' and srcid in mapping and js['nodes'][srcid].get('name','').lower().endswith(('hips','pelvis','hip'))):continue
            sampler=anim['samplers'][channel['sampler']]
            if sampler.get('interpolation','LINEAR') not in ('LINEAR','STEP'):continue
            times=read_accessor(js,blob,sampler['input'])
            rotations=read_accessor(js,blob,sampler['output'])
            if len(times)!=len(rotations) or len(times)<2:continue
            tnode=mapping[srcid]
            if path == 'rotation':
                qrest=js['nodes'][srcid].get('rotation',[0,0,0,1])
                targetrest=tnode['rest']
                # Transfer angular delta via each skeleton's parent rest frame.
                # This corrects different rest-space bone orientations; it does
                # not solve different proportions, ground contact or collision.
                sp=source_global[source_parents[srcid]] if srcid in source_parents else [0,0,0,1]
                tp=tnode['parent_world']
                vals=[]
                for q in rotations:
                    source_delta=quat_mul(q,quat_inv(qrest))
                    world_delta=quat_mul(sp,quat_mul(source_delta,quat_inv(sp)))
                    target_delta=quat_mul(quat_inv(tp),quat_mul(world_delta,tp))
                    vals.append(quat_norm(quat_mul(target_delta,targetrest)))
                values=vals
            else:
                srest=js['nodes'][srcid].get('translation',[0,0,0])
                trest=tnode['pos']
                ratio=trest[1]/srest[1] if abs(srest[1])>0.01 else 1.0
                ratio=max(0.35,min(2.5,abs(ratio)))
                values=[[trest[i]+(v[i]-srest[i])*ratio for i in range(3)] for v in rotations]
            channels.append({'node':tnode['id'],'path':path,'times':[v[0] for v in times],'values':values})
        if len(channels)>=10:
            candidate.append((name,channels))
    return candidate


def add_animation(js,buffer,animname,channels):
    name=animname;anim={'name':name,'samplers':[],'channels':[]}
    for ch in channels:
        value_type='VEC4' if ch.get('path','rotation')=='rotation' else 'VEC3'
        value_size=4 if value_type=='VEC4' else 3
        for typ,items,components in [('SCALAR',ch['times'],1),(value_type,ch['values'],value_size)]:
            data=b''.join(struct.pack('<'+'f'*components,*(v if isinstance(v,list) else [v])) for v in items)
            offset=len(buffer);buffer+=b'\0'*((-offset)%4);offset=len(buffer)
            buffer+=data
            viewidx=len(js.setdefault('bufferViews',[]))
            js['bufferViews'].append({'buffer':0,'byteOffset':offset,'byteLength':len(data)})
            accessor={'bufferView':viewidx,'componentType':5126,'count':len(items),'type':typ}
            if typ=='SCALAR':accessor.update({'min':[min(items)],'max':[max(items)]})
            accidx=len(js.setdefault('accessors',[]));js['accessors'].append(accessor)
            if typ=='SCALAR':timeidx=accidx
            else:quatidx=accidx
        sampidx=len(anim['samplers']);anim['samplers'].append({'input':timeidx,'output':quatidx,'interpolation':'LINEAR'})
        anim['channels'].append({'sampler':sampidx,'target':{'node':ch['node'],'path':ch.get('path','rotation')}})
    js.setdefault('animations',[]).append(anim)
    return buffer


def download_bundle(path=BUNDLE,url=URL):
    path.parent.mkdir(parents=True,exist_ok=True)
    if path.is_file() and zipfile.is_zipfile(path):
        if hashlib.sha256(path.read_bytes()).hexdigest() == EXPECTED_CC0_SHA256:
            return path
        raise ValueError('Cached CC0 bundle differs from published source SHA256')
    tmp=path.with_suffix('.downloading')
    try:
        with urllib.request.urlopen(urllib.request.Request(url,headers={'User-Agent':'AKARI-motion-import/0.1'}),timeout=35) as f,open(tmp,'wb') as dest:
            total=0
            while True:
                data=f.read(65536)
                if not data:break
                total+=len(data)
                if total>100_000_000:raise ValueError('Unexpected motion bundle size')
                dest.write(data)
        if not zipfile.is_zipfile(tmp):raise ValueError('Downloaded data is not a ZIP')
        digest=hashlib.sha256(tmp.read_bytes()).hexdigest()
        if digest!=EXPECTED_CC0_SHA256:
            raise ValueError('CC0 bundle SHA256 mismatch: '+digest)
        tmp.replace(path)
    finally:tmp.unlink(missing_ok=True)
    return path


def build(bundle,output=OUTPUT,target=TARGET):
    original=target.read_bytes();js,bin_data=glb_read(original)
    human=json.loads(MAP.read_text(encoding='utf8'))['bones']
    target_world,target_parents=global_rest_rotations(js['nodes'])
    targets={n.get('name',''):{'id':i,'rest':n.get('rotation',[0,0,0,1]),'pos':n.get('translation',[0,0,0]),'parent_world':target_world[target_parents[i]] if i in target_parents else [0,0,0,1]} for i,n in enumerate(js['nodes'])}
    clips={};audit=[]
    with zipfile.ZipFile(bundle) as z:
        for zfile in z.namelist():
            if not zfile.lower().endswith(('.glb','.gltf')) or z.getinfo(zfile).file_size>12_000_000:continue
            try:
                for name,ch in collect_motion(z.read(zfile),zfile,human,targets):
                    if name not in clips or len(ch)>len(clips[name]):clips[name]=ch
                audit.append(zfile)
            except (ValueError,KeyError,struct.error) as ex:
                audit.append(f'REJECT {zfile}: {ex}')
    required={'walk','sit','rise'}
    if not required.issubset(clips):raise ValueError(f'Missing mandatory CC0 animations {sorted(required-set(clips))}; imported {sorted(clips)}')
    for name,ch in sorted(clips.items()):bin_data=add_animation(js,bin_data,name,ch)
    js['buffers'][0]['byteLength']=len(bin_data)
    payload=glb_pack(js,bin_data)
    j,b=glb_read(payload)
    if not required.issubset({a['name'] for a in j['animations']}):raise AssertionError('Missing clips after pack')
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_bytes(payload)
    record={'source':URL,'license':'CC0 1.0 (Quaternius/KinemIQ)','source_sha256':hashlib.sha256(bundle.read_bytes()).hexdigest(),'base_model_sha256':hashlib.sha256(original).hexdigest(),'animations':[{'name':name,'animated_bones':len(clips[name])} for name in sorted(clips)],'audit':audit,'limitation':'Rest-space angular delta transfer; not validated against Godot runtime, ground contacts, or actual visual naturalness'}
    (output.parent/'akari_animated.provenance.json').write_text(json.dumps(record,ensure_ascii=False,indent=2),encoding='utf8')
    return record


def main():
    p=argparse.ArgumentParser();p.add_argument('--bundle',type=Path,default=BUNDLE);p.add_argument('--no-download',action='store_true');p.add_argument('--output',type=Path,default=OUTPUT);args=p.parse_args()
    if not args.bundle.is_file():
        if args.no_download:raise SystemExit('FAIL: bundle unavailable; AKARI original unchanged')
        print('Downloading the CC0 motion bundle from official release...')
        download_bundle(args.bundle)
    result=build(args.bundle,args.output)
    print('PASS: AKARI Mocap clips:',[(a['name'],a['animated_bones']) for a in result['animations']]);print('Output:',args.output)

if __name__=='__main__':main()
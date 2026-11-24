#!/usr/bin/env python3
"""AKARI experimental CC0 humanoid clip importer. Original VRM/GLB remains untouched.

Downloads the published CC0 35-clip KinemIQ bundle, maps animated local bone
rotations onto AKARI's VRM humanoid bone list, and builds a separate GLB.
This is an approximate rotation retarget, not IK/contact correction; reject
incompatible clips rather than silently invent a successful import.
"""
from __future__ import annotations
import argparse, copy, hashlib, json, math, re, struct, urllib.request, urllib.error, zipfile, socket, sys
from pathlib import Path

URL = 'https://huggingface.co/datasets/Kinemiq/free-game-animations/resolve/main/KinemIQ_Free_Animations_CC0_GLB.zip'
# Both sources carry the same exact hash-pinned bytes. The mirror is not the publisher.
DOWNLOAD_SOURCES = (
    ('Hugging Face / official', URL),
    ('Hugging Face / download redirect', URL + '?download=true'),
    ('Hugging Face community mirror', 'https://hf-mirror.com/datasets/Kinemiq/free-game-animations/resolve/main/KinemIQ_Free_Animations_CC0_GLB.zip'),
)
SOURCE_PAGE = 'https://huggingface.co/datasets/Kinemiq/free-game-animations/blob/main/KinemIQ_Free_Animations_CC0_GLB.zip'
EXPECTED_CC0_SHA256 = '64ca5ab4a92c9da079c5e88ca509391b0a58a24ccf2a1307b16e2f0fc6b23ffc'  # source-published ZIP hash
USER_SUPPLIED_CC0_SHA256 = '55365fc4c2c472ab24fc735139e5ae0d1e2d0e54ef383877623290e0379b947e'  # exact ZIP supplied by Kim 2026-11-24; source authenticity not cryptographically verified
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
    # Fingertip articulation is important for a lifelike seated/idle pose.
    for target_finger, source_finger in [('Index','index'),('Middle','middle'),('Ring','ring'),('Little','pinky')]:
        for stage,number in [('Proximal','01'),('Intermediate','02'),('Distal','03')]:
            ALI[side+target_finger+stage] = [f'{source_finger}_{number}_{short}']
    for stage,number in [('Metacarpal','01'),('Proximal','02'),('Distal','03')]:
        ALI[side+'Thumb'+stage] = [f'thumb_{number}_{short}']


def quat_mul(a,b):
    x,y,z,w=a;X,Y,Z,W=b
    return [w*X+x*W+y*Z-z*Y,w*Y-x*Z+y*W+z*X,w*Z+x*Y-y*X+z*W,w*W-x*X-y*Y-z*Z]

def quat_inv(q):
    d=sum(x*x for x in q)
    return [-q[0]/d,-q[1]/d,-q[2]/d,q[3]/d] if d else [0,0,0,1]

def quat_norm(q):
    d=math.sqrt(sum(x*x for x in q))
    return [x/d for x in q] if d else [0,0,0,1]

def quat_rotate(q,vec):
    # Rotate a Vector3 by normalized quaternion; this handles Z-up -> Y-up.
    v=quat_mul(quat_mul(quat_norm(q),[vec[0],vec[1],vec[2],0.0]),quat_inv(quat_norm(q)))
    return v[:3]


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
    # Prefer exactly named, everyday-life clips; do not substitute zombie/sword
    # animations merely because their names contain walk/idle.
    filename = normalize(Path(str(raw).split(' ')[0]).stem)
    by_file = {
        'walkforwards':'walk', 'walkformal':'walk_formal', 'walkcarrying':'walk_carry',
        'walkcrouchedforwards':'walk_crouched', 'runjogforwards':'jog',
        'idle': 'idle', 'idlerelaxed':'idle', 'idlesitting':'seated_idle',
        'idletalking':'talk_idle', 'idlearmsfoldedwaiting':'wait',
        'idlecrouching':'crouched_idle', 'actionsitdown':'sit',
        'actionstandupfromsitting':'rise', 'actioninteract':'interact',
        'actionpickupfromatable':'pick_up', 'actiondance':'dance',
    }
    if filename in by_file:
        return by_file[filename]
    if filename.startswith(('walkzombie','idlezombie','idlesword','idlewithasword', 'punch', 'strike', 'sword','hit','death')):
        return None
    text=normalize(raw)
    if ('sitdown' in text or 'sittingdown' in text or 'standtosit' in text): return 'sit'
    if ('standup' in text or 'standfromsit' in text or 'sittostand' in text or 'getupfromsit' in text): return 'rise'
    if 'walk' in text and 'back' not in text and 'side' not in text: return 'walk'
    if 'idle' in text and 'sit' in text: return 'seated_idle'
    if 'idle' in text or 'standrelaxed' in text: return 'idle'
    if 'stretch' in text: return 'stretch'
    if 'turn' in text and ('left' in text or 'right' in text): return 'turn'
    return None


def collect_motion(source: bytes, source_label: str, humanoid: dict, target_nodes, source_reference_nodes=None):
    js,blob=glb_read(source)
    if not js.get('animations'): return []
    available={normalize(n.get('name','')):i for i,n in enumerate(js['nodes'])}
    # All 35 GLBs contain their animation's first pose in node.rotation.
    # Using that pose as the rest erases seated/talking posture and makes the
    # beginning of each clip falsely resemble the same T-pose. Anchor all
    # animations to the common, standing KinemIQ mannequin skeleton instead.
    reference=source_reference_nodes or js['nodes']
    reference_by_name={normalize(n.get('name','')):i for i,n in enumerate(reference)}
    source_global,source_parents=global_rest_rotations(reference)
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
                source_name=normalize(js['nodes'][srcid].get('name',''))
                refid=reference_by_name.get(source_name)
                if refid is None: continue
                qrest=reference[refid].get('rotation',[0,0,0,1])
                targetrest=tnode['rest']
                # The AKARI VRM base is T-pose. Keep its approved neutral arms
                # relaxed instead of mapping the source idle back to a T-pose.
                if source_name in ('upperarml','upperarmr'):
                    angle=math.radians(79.0 if source_name.endswith('l') else -79.0)
                    targetrest=quat_mul(targetrest,[0.0,0.0,math.sin(angle/2.0),math.cos(angle/2.0)])
                # Transfer angular delta via each skeleton's parent rest frame.
                # This corrects different rest-space bone orientations; it does
                # not solve different proportions, ground contact or collision.
                sp=source_global[source_parents[refid]] if refid in source_parents else [0,0,0,1]
                tp=tnode['parent_world']
                vals=[]
                for q in rotations:
                    source_delta=quat_mul(q,quat_inv(qrest))
                    world_delta=quat_mul(sp,quat_mul(source_delta,quat_inv(sp)))
                    target_delta=quat_mul(quat_inv(tp),quat_mul(world_delta,tp))
                    vals.append(quat_norm(quat_mul(target_delta,targetrest)))
                values=vals
            else:
                source_name=normalize(js['nodes'][srcid].get('name',''))
                refid=reference_by_name.get(source_name)
                if refid is None: continue
                srest=reference[refid].get('translation',[0,0,0])
                trest=tnode['pos']
                # KinemIQ pelvis translation is Z-up (source pelvis Z~0.913 m),
                # while AKARI pelvis is Y-up (~0.909 m). Source root provides
                # -90deg X coordinate conversion. An animation's first-frame
                # pelvis may be ALREADY SEATED; never subtract that as rest.
                sp=source_global[source_parents[refid]] if refid in source_parents else [0,0,0,1]
                tp=tnode['parent_world']
                standing_height=max(abs(srest[2]),0.1)
                ratio=max(0.4,min(2.0,abs(trest[1])/standing_height))
                values=[]
                for v in rotations:
                    delta=quat_rotate(quat_inv(tp),quat_rotate(sp,[v[i]-srest[i] for i in range(3)]))
                    # NERVA owns horizontal root movement; preserve vertical
                    # weight transfer and seated backward shift only.
                    if name in ('walk','walk_formal','walk_carry','walk_crouched','jog'):
                        delta[0]=max(-0.035,min(0.035,delta[0]))
                        delta[2]=0.0
                    values.append([trest[i]+delta[i]*ratio for i in range(3)])
            channels.append({'node':tnode['id'],'path':path,'times':[v[0] for v in times],'values':values})
        if len(channels)>=10:
            candidate.append((name,channels))
    return candidate


def align_seated_hip_contacts(clips, hip_id):
    """Keep one physical seat position through sit -> seated idle -> rise.

    The source's seated-idle and stand-up files each start with a different
    pelvis Y/Z origin; raw retargeting teleports the pelvis between clips.
    Correct only the small horizontal root offset, not the captured skeleton.
    """
    def hip(name):
        return next((channel for channel in clips.get(name, ()) if
                     channel['node']==hip_id and channel['path']=='translation'), None)
    sit, hold, rise, stand = (hip(n) for n in ('sit','seated_idle','rise','idle'))
    if not all((sit, hold, rise, stand)):
        return {'enabled':False,'reason':'missing_hip_track'}
    end_sit = sit['values'][-1]
    first_hold = hold['values'][0]
    offsets = {axis:end_sit[axis]-first_hold[axis] for axis in (0,2)}
    for value in hold['values']:
        for axis in (0,2):value[axis]+=offsets[axis]
    last_hold=hold['values'][-1]
    start_rise=rise['values'][0]
    end_rise=rise['values'][-1]
    rest=stand['values'][0]
    start_offsets={axis:last_hold[axis]-start_rise[axis] for axis in (0,2)}
    end_offsets={axis:rest[axis]-end_rise[axis] for axis in (0,2)}
    times=rise['times']
    span=max(0.001,times[-1]-times[0])
    for i,value in enumerate(rise['values']):
        t=max(0.0,min(1.0,(times[i]-times[0])/span))
        for axis in (0,2):
            value[axis]+=start_offsets[axis]*(1-t)+end_offsets[axis]*t
    return {'enabled':True,'seat_horizontal_offset_m':[round(offsets[a],4) for a in (0,2)],
            'sit_hold_continuity_m':max(abs(end_sit[a]-hold['values'][0][a]) for a in range(3)),
            'hold_rise_continuity_m':max(abs(last_hold[a]-rise['values'][0][a]) for a in range(3)),
            'rise_stand_continuity_m':max(abs(rise['values'][-1][a]-rest[a]) for a in range(3))}


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


def check_bundle(path: Path) -> None:
    """Verify official CC0 distribution bytes; never trust a mirror by name."""
    if not path.is_file():
        raise FileNotFoundError(f'Motion source not found: {path}')
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    if digest not in (EXPECTED_CC0_SHA256, USER_SUPPLIED_CC0_SHA256):
        raise ValueError(f'Motion ZIP SHA256 mismatch: {digest} (not the original published fingerprint or Kim-uploaded audited archive)')
    if not zipfile.is_zipfile(path):
        raise ValueError('Motion source matches expected name, but is not ZIP')
    with zipfile.ZipFile(path) as z:
        if z.testzip() is not None:
            raise ValueError('Motion ZIP contains corrupt members')
        if sum(f.file_size for f in z.infolist()) > 500_000_000:
            raise ValueError('Motion ZIP exceeds uncompressed safety limit')
        if not any(n.lower().endswith('.glb') for n in z.namelist()):
            raise ValueError('Motion ZIP contains no GLB animations')
        if digest == USER_SUPPLIED_CC0_SHA256:
            # This is the uploaded copy (with __MACOSX metadata), not the
            # byte-identical publisher ZIP. Verify its actual source contents.
            actual=[n for n in z.namelist() if n.lower().endswith('.glb') and '/._' not in n and not n.startswith('__MACOSX/')]
            if len(actual) != 36:
                raise ValueError(f'User-supplied archive: expected mannequin + 35 GLBs; got {len(actual)}')
            license_notes=[n for n in z.namelist() if n.lower().endswith('/readme.txt')]
            if len(license_notes)!=1 or not all(t in z.read(license_notes[0]).decode('utf8') for t in ('CC0 1.0','Quaternius')):
                raise ValueError('User-supplied archive license README inconsistent')


def download_bundle(path=BUNDLE, sources=DOWNLOAD_SOURCES, opener=None):
    """Try official CDN and a hash-verified mirror; report each real error.

    A local verified ZIP is always preferred and works without Internet.
    """
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.is_file():
        check_bundle(path)
        print('CC0 archive already cached and SHA256 verified:', path, flush=True)
        return path
    tmp = path.with_suffix('.downloading')
    opener = opener or urllib.request.urlopen
    diagnostics = []
    for label, url in sources:
        tmp.unlink(missing_ok=True)
        print('MOTION_DOWNLOAD_TRY:', label, url, flush=True)
        try:
            request = urllib.request.Request(url, headers={'User-Agent': 'AKARI-Motion-Importer/0.2'})
            with opener(request, timeout=40) as response, open(tmp, 'wb') as output:
                if response.headers.get('Content-Type', '').lower().startswith('text/html'):
                    raise ValueError('Server returned HTML instead of a motion archive')
                total = 0
                while True:
                    data = response.read(65536)
                    if not data: break
                    total += len(data)
                    if total > 100_000_000: raise ValueError('Archive exceeds 100 MB')
                    output.write(data)
            check_bundle(tmp)
            tmp.replace(path)
            print('MOTION_DOWNLOAD_OK:', label, 'bytes=', path.stat().st_size, flush=True)
            return path
        except (OSError, ValueError, zipfile.BadZipFile, urllib.error.URLError) as exc:
            error = f'{label}: {type(exc).__name__}: {exc}'
            diagnostics.append(error)
            print('MOTION_DOWNLOAD_FAIL:', error, file=sys.stderr, flush=True)
        finally:
            tmp.unlink(missing_ok=True)
    raise RuntimeError('All motion sources failed\n' + '\n'.join(diagnostics) +
                       '\nOffline alternative: download the exact GLB ZIP via ' + SOURCE_PAGE +
                       '\nPlace it at ' + str(path) + ' and run again; SHA256 is checked.')


def build(bundle,output=OUTPUT,target=TARGET):
    check_bundle(Path(bundle))
    original=target.read_bytes();js,bin_data=glb_read(original)
    human=json.loads(MAP.read_text(encoding='utf8'))['bones']
    target_world,target_parents=global_rest_rotations(js['nodes'])
    targets={n.get('name',''):{'id':i,'rest':n.get('rotation',[0,0,0,1]),'pos':n.get('translation',[0,0,0]),'parent_world':target_world[target_parents[i]] if i in target_parents else [0,0,0,1]} for i,n in enumerate(js['nodes'])}
    clips={};audit=[]
    with zipfile.ZipFile(bundle) as z:
        mannequin=next((name for name in z.namelist() if name.endswith('/KinemIQ_Mannequin.glb') and not name.startswith('__MACOSX/')),None)
        source_ref=glb_read(z.read(mannequin))[0]['nodes'] if mannequin else None
        for zfile in z.namelist():
            if not zfile.lower().endswith('.glb') or zfile.startswith('__MACOSX/') or '/._' in zfile or z.getinfo(zfile).file_size>12_000_000:continue
            try:
                for name,ch in collect_motion(z.read(zfile),zfile,human,targets,source_reference_nodes=source_ref):
                    if name not in clips or len(ch)>len(clips[name]):clips[name]=ch
                audit.append(zfile)
            except (ValueError,KeyError,struct.error) as ex:
                audit.append(f'REJECT {zfile}: {ex}')
    required={'walk','sit','rise','idle','seated_idle'}
    if not required.issubset(clips):raise ValueError(f'Missing mandatory CC0 animations {sorted(required-set(clips))}; imported {sorted(clips)}')
    hip_node=targets[human['hips']]['id']
    pose_contact=align_seated_hip_contacts(clips,hip_node)
    if not pose_contact.get('enabled') or max(pose_contact[k] for k in ('sit_hold_continuity_m','hold_rise_continuity_m','rise_stand_continuity_m'))>0.04:
        raise ValueError(f'Unsafe seated/standing pelvis discontinuity: {pose_contact}')
    for name,ch in sorted(clips.items()):bin_data=add_animation(js,bin_data,name,ch)
    js['buffers'][0]['byteLength']=len(bin_data)
    payload=glb_pack(js,bin_data)
    j,b=glb_read(payload)
    if not required.issubset({a['name'] for a in j['animations']}):raise AssertionError('Missing clips after pack')
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_bytes(payload)
    sha=hashlib.sha256(bundle.read_bytes()).hexdigest()
    record={'source':URL,'license':'CC0 1.0 (per embedded source README, Quaternius/KinemIQ)','source_sha256':sha,'source_status':'user_supplied_audited_zip' if sha==USER_SUPPLIED_CC0_SHA256 else 'publisher_pinned_zip', 'base_model_sha256':hashlib.sha256(original).hexdigest(),'seat_continuity':pose_contact,'animations':[{'name':name,'animated_bones':len(clips[name]),'rotation_tracks':sum(c['path']=='rotation' for c in clips[name]),'translation_tracks':sum(c['path']=='translation' for c in clips[name])} for name in sorted(clips)],'audit':audit,'limitation':'Calibrated relative pose with Y-up pelvis mapping; Godot runtime, foot IK/contact, and visual naturalness still require observation'}
    (output.parent/'akari_animated.provenance.json').write_text(json.dumps(record,ensure_ascii=False,indent=2),encoding='utf8')
    return record


def main():
    p=argparse.ArgumentParser();p.add_argument('--bundle',type=Path,default=BUNDLE);p.add_argument('--no-download',action='store_true');p.add_argument('--output',type=Path,default=OUTPUT);args=p.parse_args()
    if not args.bundle.is_file():
        if args.no_download:
            raise SystemExit('FAIL: motion archive missing: ' + str(args.bundle))
        print('CC0 motion archive absent. Downloading with SHA256 validation...')
        try:
            download_bundle(args.bundle)
        except RuntimeError as exc:
            raise SystemExit('FAIL: ' + str(exc))
    check_bundle(args.bundle)
    result=build(args.bundle,args.output)
    print('PASS: AKARI Mocap clips:',[(a['name'],a['animated_bones']) for a in result['animations']]);print('Output:',args.output)

if __name__=='__main__':main()
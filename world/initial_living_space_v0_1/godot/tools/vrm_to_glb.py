"""Make a VRM 1.0 GLB usable by a standard glTF 2.0 importer.

Does not change vertex, texture, skin, morph-target or mesh binary data.
VRM-only runtime extensions (MToon, spring bones, humanoid meta)
are removed from the GLB; facial names and their mappings are preserved separately.
This converter is not a VRM importer / renderer; check in Godot.
"""
import json
import struct
from pathlib import Path
import sys

VRM_EXTENSIONS = {"VRMC_vrm", "VRMC_springBone", "VRMC_materials_mtoon", "VRMC_node_constraint"}


def write_glb(input_path: Path, output_path: Path, map_path: Path):
    with input_path.open('rb') as stream:
        magic, version, declared_len = struct.unpack('<4sII', stream.read(12))
        assert magic == b'glTF' and version == 2
        src = input_path.read_bytes()
    assert len(src) == declared_len
    offset = 12
    chunks = []
    while offset < len(src):
        ln, typ = struct.unpack_from('<I4s',src,offset)
        offset += 8
        chunks.append((typ,src[offset:offset+ln]))
        offset += ln
    doc = json.loads(next(blob for typ,blob in chunks if typ == b'JSON'))
    assert 'VRMC_vrm' in doc.get('extensions', {}), 'Expected VRM 1.0'
    vrm = doc['extensions']['VRMC_vrm']
    bones = vrm['humanoid']['humanBones']
    expressions = vrm.get('expressions',{}).get('preset',{})
    node_names = {k: doc['nodes'][v['node']].get('name','') for k,v in bones.items()}
    expression_map = {}
    for key,record in expressions.items():
        binds = []
        for binding in record.get('morphTargetBinds',[]):
            node=doc['nodes'][binding['node']]
            mesh_idx=node['mesh']
            names=doc['meshes'][mesh_idx].get('extras',{}).get('targetNames',[])
            i=binding['index']
            binds.append({'node': node.get('name'), 'blend_shape': names[i] if i < len(names) else f'index_{i}', 'weight': binding.get('weight',1)})
        expression_map[key]=binds
    info={'source_name':input_path.name,'vrm_spec':vrm.get('specVersion'), 'bones':node_names,'expressions':expression_map,
          'notes':'Runtime VRM extensions removed; spring-bone physics and MToon shaders are not preserved by this conversion.'}
    map_path.write_text(json.dumps(info,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    # Remove VRM-specific glTF extensions, keep standard KHR ones.
    doc['extensionsUsed']=[x for x in doc.get('extensionsUsed',[]) if x not in VRM_EXTENSIONS]
    if 'extensionsRequired' in doc:
        doc['extensionsRequired']=[x for x in doc['extensionsRequired'] if x not in VRM_EXTENSIONS]
    doc['extensions']={k:v for k,v in doc.get('extensions',{}).items() if k not in VRM_EXTENSIONS}
    if not doc['extensions']: doc.pop('extensions')
    for material in doc.get('materials',[]):
        ext=material.get('extensions',{})
        if ext:
            material['extensions']={k:v for k,v in ext.items() if k not in VRM_EXTENSIONS}
            if not material['extensions']: material.pop('extensions')
    # Only JSON metadata changed. All GLB BIN payload bytes remain intact.
    new_json=json.dumps(doc,ensure_ascii=False,separators=(',',':')).encode('utf-8')
    new_json+=b' ' * ((-len(new_json))%4)
    new_chunks=[]
    for typ,blob in chunks:
        new_chunks.append((typ,new_json if typ==b'JSON' else blob))
    total=12+sum(8+len(blob) for _,blob in new_chunks)
    with output_path.open('wb') as f:
        f.write(struct.pack('<4sII',b'glTF',2,total))
        for typ,blob in new_chunks:
            f.write(struct.pack('<I4s',len(blob),typ))
            f.write(blob)
    return info

if __name__=='__main__':
    source=Path(sys.argv[1])
    target=Path(sys.argv[2])
    details=Path(sys.argv[3])
    write_glb(source,target,details)
    print('Wrote',target,target.stat().st_size)

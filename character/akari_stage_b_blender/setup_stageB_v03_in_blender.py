"""Build an editable Stage-B Blender review scene around a rejected, new-from-zero mesh.
Usage: blender -b --python tools/setup_stageB_in_blender.py
This script DOES NOT sculpt a final AKARI likeness. It makes a Blender-native
starting document with immutable references, editable collections, comparison
cameras and repeatable four-view output. Original v0.5 assets are not imported.
"""
from __future__ import annotations
import bpy
from pathlib import Path
import math
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / 'source' / 'AKARI_StageB_V03_NEW_FACE_HAIR_UNAPPROVED.glb'
REF = ROOT / 'reference'
OUT = ROOT / 'output'
OUT.mkdir(parents=True, exist_ok=True)

bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
for coll in list(bpy.data.collections):
    if not coll.objects and not coll.children:
        bpy.data.collections.remove(coll)


def coll_named(name):
    col = bpy.data.collections.new(name)
    bpy.context.scene.collection.children.link(col)
    return col


def move_to_collection(obj, col):
    for old in list(obj.users_collection):
        old.objects.unlink(obj)
    col.objects.link(obj)


body = coll_named('01_AKARI_EDITABLE_V03_NOT_APPROVED')
refs = coll_named('02_PRIVATE_REFERENCE_IMAGES')
cams = coll_named('03_REVIEW_CAMERAS')
lights = coll_named('04_REVIEW_LIGHTS')
settings = coll_named('05_QA_MARKERS')

before = set(bpy.data.objects)
bpy.ops.import_scene.gltf(filepath=str(SRC), import_shading='NORMALS')
new_objs = [o for o in bpy.data.objects if o not in before]
for obj in new_objs:
    move_to_collection(obj, body)
    if obj.type == 'MESH':
        obj['QA'] = 'UNAPPROVED_V03: face/hair/clothing still need artist refinement'
        for mat in obj.data.materials:
            if mat:
                mat.diffuse_color = mat.diffuse_color

# glTF's Y-up becomes Blender Z-up, scale in metres. Mesh is an editable
# comparison reference, not a promised final likeness.
mesh_objs = [o for o in new_objs if o.type == 'MESH']
if not mesh_objs:
    raise RuntimeError('No meshes imported; check GLB integrity')

try:
    bpy.context.scene.render.engine = 'BLENDER_EEVEE_NEXT'
except Exception:
    bpy.context.scene.render.engine = 'BLENDER_EEVEE'
scene = bpy.context.scene
scene.render.resolution_x = 900
scene.render.resolution_y = 1100
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = 'PNG'
scene.render.film_transparent = False
scene.world.color = (.8, .8, .8)
scene.render.image_settings.color_mode = 'RGBA'
try:
    scene.view_settings.view_transform = 'AgX'
except Exception:
    scene.view_settings.view_transform = 'Standard'

# Material-independent bright neutral comparison background.
world = scene.world or bpy.data.worlds.new('StageB_white_studio')
scene.world = world
world.use_nodes = True
bg = world.node_tree.nodes.get('Background')
if bg:
    bg.inputs['Color'].default_value = (.88, .89, .93, 1)
    bg.inputs['Strength'].default_value = .8


def make_camera(name, loc, target=(0,0,.82), scale=1.06):
    data = bpy.data.cameras.new(name)
    obj = bpy.data.objects.new(name,data)
    cams.objects.link(obj)
    obj.location = loc
    direction = Vector(target) - obj.location
    obj.rotation_euler = direction.to_track_quat('-Z','Y').to_euler()
    data.type = 'ORTHO'
    data.ortho_scale = scale*2
    data.lens = 55
    return obj

# Source glTF character faces +Z, which converts to Blender -Y.
views = {
    'front': make_camera('01_FRONT_camera',(0,-4.2,.83)),
    'side': make_camera('02_SIDE_camera',(4.2,0,.83)),
    'back': make_camera('03_BACK_camera',(0,4.2,.83)),
    'three_quarter':make_camera('04_45deg_camera',(2.95,-2.95,.87)),
}


def make_area(name, at, power, size, target=(0,0,1.05)):
    data=bpy.data.lights.new(name,'AREA')
    data.energy=power
    data.shape='DISK';data.size=size
    obj=bpy.data.objects.new(name,data)
    lights.objects.link(obj);obj.location=at
    obj.rotation_euler=(Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler()

make_area('Key_Large',(2.5,-3,3.1),600,4.0)
make_area('Fill_Large',(-2.5,-1.5,1.9),300,3.0)
make_area('Rim_Large',(0,2.5,2.9),400,3.0)

# Images are packed INSIDE the private .blend, never copied to public GitHub.
# Assign as Blender-native image empties, hidden from all renders.
for label, filename, xyz, size in [
    ('FULL_BODY_REFERENCE','full_body.jpeg',(2.2,1.3,1.05),1.9),
    ('CHARACTER_DESIGN_SHEET','design_sheet.jpeg',(3.7,1.3,1.05),1.9),
    ('SMILE_FACE_REFERENCE','smile.jpeg',(5.2,1.3,1.05),1.2),
    ('CONFUSED_FACE_REFERENCE','confused.jpeg',(6.7,1.3,1.05),1.2),
]:
    path=REF/filename
    if not path.is_file(): raise RuntimeError(f'Missing reference image: {path}')
    image=bpy.data.images.load(str(path),check_existing=True)
    image.pack()
    obj=bpy.data.objects.new(label,None)
    refs.objects.link(obj)
    obj.empty_display_type='IMAGE';obj.data=image
    obj.location=xyz;obj.empty_display_size=size
    obj.hide_render=True
    obj['Private Reference / Copyright']='Original supplied by Kim; do not push image to public GitHub'

# Dimension guides for verifying 163 cm without asserting exact likeness.
def marker(name,z):
    bpy.ops.mesh.primitive_cube_add(size=1,location=(.6,0,z))
    obj=bpy.context.object;obj.name=name
    obj.dimensions=(.20,.012,.005)
    bpy.ops.object.transform_apply(location=False, rotation=False,scale=True)
    move_to_collection(obj,settings)
    obj.hide_render=True
marker('MEASURE_163CM',1.63)
marker('MEASURE_GROUND',0)

scene['AKARI_MASTER'] = 'docs/sacred/LOCK_AKARI_CHARACTER_REBUILD_VISUAL_LIFELIKE_2026-11-25.md'
scene['STAGE_B_QA'] = 'FAIL: v03 is a sculpting candidate; no cute-character signoff'
scene['MESH_SOURCE'] = 'new-from-zero Stage B v03: v02 body scaffold + newly remodeled face and hair; no v0.5 meshes'
scene['HEIGHT_REFERENCE_METERS'] = 1.63
scene.camera = views['front']

# Activate meshes for immediate sculpt editing, not cameras/references.
bpy.ops.object.select_all(action='DESELECT')
mesh_objs[0].select_set(True)
bpy.context.view_layer.objects.active=mesh_objs[0]

blend_path=OUT/'AKARI_StageB_v03_EDITABLE_UNAPPROVED.blend'
bpy.ops.wm.save_as_mainfile(filepath=str(blend_path),compress=True)
for view_name,camera in views.items():
    scene.camera = camera
    scene.render.filepath=str(OUT/f'AKARI_StageB_BLENDER_{view_name}.png')
    bpy.ops.render.render(write_still=True)
    print(f'BLENDER_RENDER_OK {view_name} {scene.render.filepath}')
print('BLENDER_STAGEB_WORKSPACE_DONE',blend_path)
"""AKARI Room soft bed: Blender authoring/refinement step.
Use Blender's Scripting workspace or: blender -b -P tools/blender_refine_bed.py
Requires assets/akari_bed_soft_v01.glb (in supplied artifact ZIP).
Produces Blender-editable .blend and refined GLB.
"""
from pathlib import Path
import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets" / "akari_bed_soft_v01.glb"
if not SOURCE.exists():
    raise FileNotFoundError("Copy akari_bed_soft_v01.glb into world/akari_room/assets first")
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.gltf(filepath=str(SOURCE))
for obj in bpy.context.scene.objects:
    if obj.type != "MESH":
        continue
    for polygon in obj.data.polygons:
        polygon.use_smooth = True
    lower = obj.name.lower()
    if "wood" in lower or "foot" in lower:
        bevel = obj.modifiers.new("Soft wooden corners", "BEVEL")
        bevel.width = .015
        bevel.segments = 3
        bevel.limit_method = "ANGLE"
    if any(token in lower for token in ("duvet", "knitted", "pillow")):
        subdiv = obj.modifiers.new("Fabric subdivision", "SUBSURF")
        subdiv.levels = 1
        subdiv.render_levels = 2

world = bpy.context.scene.world or bpy.data.worlds.new("AKARI Warm Ambient")
bpy.context.scene.world = world
world.use_nodes = True
background = world.node_tree.nodes.get("Background")
if background:
    background.inputs["Color"].default_value = (.68, .72, .83, 1)
    background.inputs["Strength"].default_value = .6
bpy.ops.object.camera_add(location=(3.8, 2.9, 4.4))
camera = bpy.context.object
camera.name = "PreviewCamera"
camera.rotation_euler = (Vector((0,.7,0))-camera.location).to_track_quat("-Z","Y").to_euler()
camera.data.type = "ORTHO"
camera.data.ortho_scale = 4.25
bpy.context.scene.camera = camera
bpy.ops.object.light_add(type="AREA", location=(-2.7,4.6,1.6))
light = bpy.context.object
light.name = "PreviewSoftbox"
light.data.energy = 480
light.data.shape = "RECTANGLE"
light.data.size = 4
light.data.size_y = 3
light.rotation_euler = (Vector((0,.55,0))-light.location).to_track_quat("-Z","Y").to_euler()
bpy.context.scene.render.engine = "CYCLES"
bpy.context.scene.cycles.samples = 32
bpy.context.scene.render.resolution_x = 1100
bpy.context.scene.render.resolution_y = 820
bpy.context.scene.render.resolution_percentage = 100
bpy.context.scene.render.filepath = str(ROOT / "assets" / "bed_blender_preview.png")
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT / "assets" / "akari_bed_soft_v01.blend"))
bpy.ops.render.render(write_still=True)
bpy.ops.object.select_all(action="DESELECT")
for obj in bpy.context.scene.objects:
    if obj.type == "MESH":
        obj.select_set(True)
bpy.ops.export_scene.gltf(
    filepath=str(ROOT / "assets" / "akari_bed_refined.glb"),
    export_format="GLB",
    use_selection=True,
    export_apply=True,
)
print("AKARI Bed Blender refinement complete")

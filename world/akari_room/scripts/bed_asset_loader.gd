extends RefCounted
## Non-destructive optional Blender/glTF art asset override.
## If the GLB is absent, the existing room remains usable.
const BED_GLB = "res://assets/akari_bed_soft_v01.glb"

static func apply(room: Node3D) -> bool:
	if not ResourceLoader.exists(BED_GLB):
		return false
	var packed = load(BED_GLB)
	if not (packed is PackedScene):
		push_warning("AKARI Bed GLB did not import as PackedScene; keeping original bed")
		return false
	var furniture := room.get_node_or_null("Furniture") as Node3D
	if furniture == null:
		return false
	var original := furniture.get_node_or_null("bed") as Node3D
	if original == null:
		return false
	var new_bed := (packed as PackedScene).instantiate() as Node3D
	if new_bed == null:
		return false
	new_bed.name = "BlenderBed"
	furniture.add_child(new_bed)
	new_bed.position = original.position
	# Preserve old node and its static collisions as a temporary physics proxy.
	# Only its visible primitive meshes are hidden.
	original.visible = false
	return true

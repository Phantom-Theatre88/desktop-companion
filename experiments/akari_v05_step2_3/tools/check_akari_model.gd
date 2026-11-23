extends SceneTree
# Strict runtime resource check, not a scene launch check.
const AVATAR := "res://avatar/akari_animated.glb"

func _initialize() -> void:
    if not ResourceLoader.exists(AVATAR):
        _fail("GLB is not imported/available in this project: " + AVATAR)
        return
    var resource: Resource = ResourceLoader.load(AVATAR)
    if resource == null or not (resource is PackedScene):
        _fail("GLB did not load as PackedScene; inspect Godot Import dock")
        return
    var instance: Node = (resource as PackedScene).instantiate()
    if instance == null or not (instance is Node3D):
        _fail("GLB root is not Node3D")
        return
    var stats := {"meshes": 0, "skeletons": 0, "max_bones": 0, "morphs": 0, "mocap_clips": []}
    _inspect(instance, stats)
    print("AKARI_MODEL_CHECK meshes=%d skeletons=%d max_bones=%d face_morphs=%d" % [stats["meshes"], stats["skeletons"], stats["max_bones"], stats["morphs"]])
    instance.free()
    if stats["meshes"] < 1 or stats["skeletons"] < 1 or stats["max_bones"] < 6 or stats["morphs"] < 1:
        _fail("GLB is missing required actual mesh/skeleton/face morph components")
        return
    for required in ["walk", "sit", "rise"]:
        var found := false
        for a in stats["mocap_clips"]:
            if str(a).to_lower() == required or str(a).to_lower().ends_with("/" + required):
                found = true
        if not found:
            _fail("Mocap clip missing: " + required + "; cannot claim natural movement")
            return
    print("PASS: AKARI actual GLB mesh, skeleton, face morph, mocap clips resource load")
    quit(0)

func _inspect(node: Node, stats: Dictionary) -> void:
    if node is AnimationPlayer:
        stats["mocap_clips"].append_array((node as AnimationPlayer).get_animation_list())
    if node is Skeleton3D:
        stats["skeletons"] += 1
        stats["max_bones"] = maxi(stats["max_bones"], (node as Skeleton3D).get_bone_count())
    if node is MeshInstance3D:
        var mesh_node := node as MeshInstance3D
        if mesh_node.mesh != null:
            stats["meshes"] += 1
            stats["morphs"] = maxi(stats["morphs"], mesh_node.mesh.get_blend_shape_count())
    for child in node.get_children():
        _inspect(child, stats)

func _fail(message: String) -> void:
    printerr("FAIL: AKARI_MODEL_CHECK " + message)
    quit(2)
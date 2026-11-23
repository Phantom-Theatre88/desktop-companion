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
        var driven_bones := 0
        for entry in stats["mocap_clips"]:
            var name := str(entry.get("name", "")).to_lower()
            if name == required or name.ends_with("/" + required):
                found = true
                driven_bones = maxi(driven_bones, int(entry.get("bone_tracks", 0)))
        if not found:
            _fail("Mocap clip missing: " + required)
            return
        if driven_bones < 8:
            _fail("Mocap clip " + required + " has only " + str(driven_bones) + " rotation tracks. Real skeleton motion NOT verified")
            return
    print("PASS: AKARI actual GLB mesh, skeleton, face morph, mocap clips resource load")
    quit(0)

func _inspect(node: Node, stats: Dictionary) -> void:
    if node is AnimationPlayer:
        var player := node as AnimationPlayer
        for clip in player.get_animation_list():
            var animation := player.get_animation(clip)
            var tracks := 0
            if animation != null:
                for track_idx in animation.get_track_count():
                    if animation.track_get_type(track_idx) == Animation.TYPE_ROTATION_3D:
                        tracks += 1
            stats["mocap_clips"].append({"name": str(clip), "bone_tracks": tracks})
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
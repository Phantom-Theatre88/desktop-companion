extends SceneTree
var lit_count: int = 0
var unlit_highlights: int = 0
var shadow_lights: int = 0
func _initialize() -> void:
    call_deferred("verify")
func verify() -> void:
    var packed := load("res://scenes/room_integration.tscn") as PackedScene
    var instance := packed.instantiate()
    root.add_child(instance)
    await process_frame
    await process_frame
    assert(instance.room_loaded)
    assert(instance.pbr_surface_count > 50)
    assert(instance._pbr_cache.size() == 8)
    for material in instance._pbr_cache.values():
        assert(material.albedo_texture != null)
        assert(material.normal_texture != null)
        assert(material.roughness_texture != null)
    var found_environment: bool = false
    for node in instance.get_children():
        if node is WorldEnvironment:
            assert(node.environment.sdfgi_enabled)
            assert(node.environment.ssao_enabled)
            found_environment = true
    assert(found_environment)
    print("PBR_SETUP_CHECKS surfaces=",instance.pbr_surface_count," texture_families=",instance._pbr_cache.size())
    assert(instance.character != null)
    assert(instance._idle_time > 0.0)
    inspect_avatar(instance.character)
    inspect_lights(instance)
    assert(lit_count >= 10)
    assert(shadow_lights == 2)
    var bottom: float = instance._collect_character_bottom(instance.character, INF)
    assert(absf(bottom - 0.031) < 0.003)
    print("SHARED_LIGHT_CHECKS passed lit_surfaces=",lit_count," eye_highlights=",unlit_highlights," shadow_lights=",shadow_lights," static_bottom=",bottom)
    instance.queue_free()
    await process_frame
    quit(0)
func inspect_avatar(node: Node) -> void:
    if node is MeshInstance3D:
        var mi := node as MeshInstance3D
        for surface in range(mi.mesh.get_surface_count()):
            var material := mi.get_active_material(surface) as BaseMaterial3D
            assert(material != null)
            assert(not material.emission_enabled)
            if material.shading_mode == BaseMaterial3D.SHADING_MODE_PER_PIXEL:
                lit_count += 1
            else:
                assert(material.resource_name.to_lower().contains("eyehighlight"))
                unlit_highlights += 1
    for child in node.get_children():
        inspect_avatar(child)
func inspect_lights(node: Node) -> void:
    if node is Light3D and node.shadow_enabled:
        shadow_lights += 1
    for child in node.get_children():
        inspect_lights(child)

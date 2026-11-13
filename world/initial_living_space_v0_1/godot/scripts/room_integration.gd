extends "res://scripts/main.gd"
# Preserve the original Character script, skeleton, textures and idle logic.
# This is a room/Character co-display test; no locomotion or Ghost integration.
var room_loaded: bool = false
var room_message: Label

func _ready() -> void:
    super._ready()
    $AvatarAnchor.position = Vector3(0.0, 0.03, -2.95)
    $Camera3D.position = Vector3(0.05, 1.43, 1.25)
    $Camera3D.fov = 41.11209
    $Camera3D.look_at(Vector3(0.05, 1.09, -3.55), Vector3.UP)
    $Camera3D.far = 200.0
    $DirectionalLight3D.light_energy = 0.30
    $DirectionalLight3D.shadow_enabled = false
    _room_environment()
    _load_room_geometry()
    _light_character_materials(character)
    var inspection_light := DirectionalLight3D.new()
    inspection_light.name = "TowerInspectionLight"
    inspection_light.layers = 2
    inspection_light.light_cull_mask = 2
    inspection_light.light_energy = 0.85
    inspection_light.light_color = Color(0.93,0.96,1.0)
    inspection_light.rotation_degrees = Vector3(-25,-35,0)
    inspection_light.shadow_enabled = false
    inspection_light.visible = false
    add_child(inspection_light)
    _align_character_to_floor()
    $AvatarAnchor.position.y -= 0.0156
    # Original diagnostics remain accessible, initially hidden to expose the room.
    for child in get_children():
        if child is CanvasLayer:
            child.visible = false
    var overlay := CanvasLayer.new()
    add_child(overlay)
    room_message = Label.new()
    overlay.add_child(room_message)
    room_message.position = Vector2(12, 12)
    room_message.text = "AKARI / Living details v10 | Tab: controls | 1: room view | 2: character | 3: feet | 0: hide/show AKARI | 4: tower detail"
    if not room_loaded:
        room_message.text += "\nROOM MISSING: run export_room_for_godot.py in Blender first."
    print("ROOM_INTEGRATION avatar_loaded=", character != null, " room_loaded=", room_loaded)

func _load_room_geometry() -> void:
    var path := "res://room/room.glb"
    if not ResourceLoader.exists(path):
        push_warning("Room GLB missing. Character remains available; integration is not complete.")
        return
    var packed := load(path) as PackedScene
    if packed == null:
        push_error("Room GLB could not be loaded.")
        return
    var room := packed.instantiate() as Node3D
    if room == null:
        push_error("Room root is not Node3D.")
        return
    room.name = "ImportedRoom"
    add_child(room)
    _room_material_visibility(room)
    # Original Blender PBR is baked into room.glb; do not replace it with generated grain.
    _refine_transparent_fabric(room)
    # City is rebuilt as a separate layered scene.
    # Plant geometry is supplied by living_details_v10.
    load("res://scripts/interior_finish_v7.gd").new().apply(self, room)
    load("res://scripts/city_v8.gd").new().build(self, room)
    load("res://scripts/living_details_v10.gd").new().build(self, room)
    # GLB exporter transforms Blender Z-up to Godot Y-up in meters.
    room_loaded = true

func _room_environment() -> void:
    var world := WorldEnvironment.new()
    var environment := Environment.new()
    environment.background_mode = Environment.BG_SKY
    var sky_resource := Sky.new()
    var sky_material := ProceduralSkyMaterial.new()
    sky_material.sky_top_color = Color(0.34, 0.53, 0.78)
    sky_material.sky_horizon_color = Color(0.62, 0.73, 0.88)
    sky_material.ground_horizon_color = Color(0.62, 0.73, 0.88)
    sky_material.ground_bottom_color = Color(0.27, 0.29, 0.34)
    sky_material.sky_curve = 0.10
    sky_resource.sky_material = sky_material
    environment.sky = sky_resource
    environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
    environment.ambient_light_color = Color(1.0, 0.95, 0.88)
    environment.ambient_light_energy = 0.20
    environment.tonemap_mode = Environment.TONE_MAPPER_FILMIC
    # Forward+ GI provides room bounce lighting without fixing WORLD to one time.
    environment.sdfgi_enabled = true
    environment.sdfgi_use_occlusion = true
    environment.sdfgi_read_sky_light = false
    environment.sdfgi_min_cell_size = 0.15
    environment.sdfgi_energy = 0.75
    environment.ssao_enabled = true
    environment.ssao_radius = 0.35
    environment.ssao_intensity = 1.1
    environment.ssao_power = 1.2
    world.environment = environment
    add_child(world)
    var reflection := ReflectionProbe.new()
    reflection.name = "RoomReflectionProbe"
    reflection.position = Vector3(0, 1.25, -2.10)
    reflection.size = Vector3(3.9, 2.5, 4.2)
    reflection.box_projection = true
    reflection.intensity = 0.35
    add_child(reflection)
    var ceiling_lamp := OmniLight3D.new()
    ceiling_lamp.position = Vector3(0, 2.12, -2.65)
    ceiling_lamp.light_color = Color(1.0, 0.72, 0.46)
    ceiling_lamp.light_energy = 0.25
    ceiling_lamp.shadow_enabled = true
    ceiling_lamp.shadow_bias = 0.04
    ceiling_lamp.shadow_opacity = 0.30
    ceiling_lamp.shadow_blur = 3.0
    ceiling_lamp.light_specular = 0.25
    ceiling_lamp.shadow_normal_bias = 0.4
    ceiling_lamp.omni_range = 5.5
    add_child(ceiling_lamp)
    var daylight := SpotLight3D.new()
    daylight.name = "SharedWindowDaylight"
    daylight.position = Vector3(0.0, 1.85, -4.12)
    daylight.light_color = Color(0.88, 0.93, 1.0)
    daylight.light_energy = 0.32
    daylight.spot_range = 7.0
    daylight.spot_angle = 75.0
    daylight.spot_attenuation = 0.55
    daylight.shadow_enabled = true
    daylight.shadow_bias = 0.04
    daylight.shadow_opacity = 0.25
    daylight.shadow_blur = 3.0
    daylight.light_specular = 0.20
    daylight.shadow_normal_bias = 0.4
    add_child(daylight)
    daylight.look_at(Vector3(0.0, 0.75, -1.2), Vector3.UP)
    # Modest shadow-free fill supports the face without adding another shadow pattern.
    var fill := OmniLight3D.new()
    fill.name = "SoftInteriorFill"
    fill.position = Vector3(0.0, 1.65, -0.35)
    fill.light_color = Color(1.0, 0.91, 0.82)
    fill.light_energy = 0.26
    fill.omni_range = 5.0
    fill.light_specular = 0.10
    fill.shadow_enabled = false
    add_child(fill)

func _unhandled_key_input(event: InputEvent) -> void:
    if not (event is InputEventKey) or not event.pressed or event.echo:
        return
    if event.keycode in [KEY_1,KEY_2,KEY_3,KEY_5,KEY_6,KEY_7]:
        $Camera3D.cull_mask = 1048575
        $TowerInspectionLight.visible = false
    if event.keycode == KEY_4:
        $Camera3D.cull_mask = 2
        $TowerInspectionLight.visible = true
        $Camera3D.position = Vector3(4.0,1.4,-28.0)
        $Camera3D.fov = 48.0
        $Camera3D.look_at(Vector3(4.0,1.4,-45.0),Vector3.UP)
    elif event.keycode == KEY_5:
        $Camera3D.position=Vector3(0.05,1.22,-2.1)
        $Camera3D.fov=36.0
        $Camera3D.look_at(Vector3(-1.60,0.96,-3.05),Vector3.UP)
    elif event.keycode == KEY_6:
        $Camera3D.position=Vector3(-0.25,1.05,-5.72)
        $Camera3D.fov=55.0
        $Camera3D.look_at(Vector3(1.04,0.78,-5.14),Vector3.UP)
    elif event.keycode == KEY_7:
        $Camera3D.position=Vector3(-0.65,1.27,-1.27)
        $Camera3D.fov=48.0
        $Camera3D.look_at(Vector3(-1.50,1.27,-1.27),Vector3.UP)
    elif event.keycode == KEY_TAB:
        for child in get_children():
            if child is CanvasLayer and child != room_message.get_parent():
                child.visible = not child.visible
    elif event.keycode == KEY_1:
        $Camera3D.position = Vector3(0.05, 1.43, 1.25)
        $Camera3D.fov = 41.11209
        $Camera3D.look_at(Vector3(0.05, 1.09, -3.55), Vector3.UP)
    elif event.keycode == KEY_0:
        $AvatarAnchor.visible = not $AvatarAnchor.visible
    elif event.keycode == KEY_3:
        $Camera3D.position = Vector3(-0.35, 1.15, -1.95)
        $Camera3D.fov = 65.0
        $Camera3D.look_at(Vector3(0.0, 0.75, -2.95), Vector3.UP)
    elif event.keycode == KEY_2:
        $Camera3D.position = Vector3(0.0, 1.35, -0.75)
        $Camera3D.fov = 42.0
        $Camera3D.look_at(Vector3(0.0, 1.05, -2.95), Vector3.UP)

# Transparent panes and thin sheer cloth should not cast opaque GL Compatibility shadows.
func _room_material_visibility(node: Node) -> void:
    if node is MeshInstance3D:
        var mesh_node := node as MeshInstance3D
        if str(mesh_node.name).begins_with("Window full height") or str(mesh_node.name).begins_with("Sheer curtain"):
            mesh_node.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
    for child in node.get_children():
        _room_material_visibility(child)

# The original avatar GLB declares KHR_materials_unlit on every material.
# Duplicate only material instances so the original avatar file stays unchanged.
func _light_character_materials(node: Node) -> void:
    if node == null:
        return
    if node is MeshInstance3D:
        var mesh_node := node as MeshInstance3D
        mesh_node.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_ON
        if mesh_node.mesh != null:
            for surface in range(mesh_node.mesh.get_surface_count()):
                var source := mesh_node.get_active_material(surface) as BaseMaterial3D
                if source == null:
                    continue
                var material := source.duplicate() as BaseMaterial3D
                # Preserve textures, transparency, mesh and facial expressions.
                material.shading_mode = BaseMaterial3D.SHADING_MODE_PER_PIXEL
                material.emission_enabled = false
                material.metallic = 0.0
                material.roughness = 0.82
                var label := source.resource_name.to_lower()
                # Prevent stylized face cards from receiving harsh hair/eyelash shadows.
                # Diffuse lighting remains enabled, as do body/floor cast shadows.
                if label.contains("face") or label.contains("eye"):
                    material.disable_receive_shadows = true
                if label.contains("face") or label.contains("body"):
                    material.albedo_color *= Color(1.0,0.96,0.92)
                if label.contains("hair"):
                    material.roughness = 0.62
                elif label.contains("cloth"):
                    material.roughness = 0.92
                if label.contains("eyehighlight"):
                    material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
                mesh_node.set_surface_override_material(surface, material)
    for child in node.get_children():
        _light_character_materials(child)

func _collect_character_bottom(node: Node, current_min: float) -> float:
    var result: float = current_min
    if node is MeshInstance3D:
        var mesh_node := node as MeshInstance3D
        if mesh_node.mesh != null:
            var world_box: AABB = mesh_node.global_transform * mesh_node.get_aabb()
            result = minf(result, world_box.position.y)
    for child in node.get_children():
        result = _collect_character_bottom(child, result)
    return result

func _align_character_to_floor() -> void:
    if character == null:
        return
    var bottom: float = _collect_character_bottom(character, INF)
    if is_finite(bottom):
        $AvatarAnchor.position.y += 0.031 - bottom
        print("CHARACTER_FLOOR_ALIGNMENT static_mesh_bottom=", bottom, " anchor_y=", $AvatarAnchor.position.y)

var pbr_surface_count: int = 0
var _pbr_cache: Dictionary = {}
func _texture_family(label: String) -> String:
    if label.begins_with("Furniture oak upright"):
        return "upright_oak"
    if label.begins_with("Furniture oak long") or label.begins_with("Oak •"):
        return "oak"
    if label.begins_with("Warm oak flooring"):
        return "floor_oak"
    if label.begins_with("Side rack"):
        return "rack_oak"
    if label.begins_with("Fine dusty rose"):
        return "rose"
    if label.begins_with("Dusty rose woven"):
        return "curtain_rose"
    if label.begins_with("Fine ivory") or label.begins_with("Ivory woven"):
        return "linen"
    if label.begins_with("Warm plaster"):
        return "plaster"
    return ""

func _apply_room_pbr(node: Node) -> void:
    if node is MeshInstance3D:
        var mi := node as MeshInstance3D
        var transparent_surface: bool = str(mi.name).begins_with("Window full height") or str(mi.name).begins_with("Sheer curtain")
        mi.gi_mode = GeometryInstance3D.GI_MODE_DISABLED if transparent_surface else GeometryInstance3D.GI_MODE_STATIC
        if mi.mesh != null:
            for index in range(mi.mesh.get_surface_count()):
                var base := mi.get_active_material(index) as BaseMaterial3D
                if base == null:
                    continue
                var family: String = _texture_family(base.resource_name)
                if family.is_empty():
                    continue
                if not _pbr_cache.has(family):
                    var material := base.duplicate() as BaseMaterial3D
                    material.albedo_color = Color.WHITE
                    material.albedo_texture = load("res://textures/" + family + "_albedo.png")
                    material.normal_enabled = true
                    material.normal_texture = load("res://textures/" + family + "_normal.png")
                    material.normal_scale = 0.35 if family in ["linen", "rose", "curtain_rose"] else 0.22
                    material.roughness = 1.0
                    material.roughness_texture = load("res://textures/" + family + "_roughness.png")
                    material.roughness_texture_channel = BaseMaterial3D.TEXTURE_CHANNEL_RED
                    material.uv1_triplanar = true
                    material.uv1_world_triplanar = true
                    material.uv1_triplanar_sharpness = 8.0
                    var tiling: float = 1.5
                    if family in ["linen", "rose", "curtain_rose"]:
                        tiling = 3.0
                    elif family == "plaster":
                        tiling = 2.0
                    material.uv1_scale = Vector3.ONE * tiling
                    material.texture_filter = BaseMaterial3D.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC
                    _pbr_cache[family] = material
                mi.set_surface_override_material(index, _pbr_cache[family])
                pbr_surface_count += 1
    for child in node.get_children():
        _apply_room_pbr(child)

func _refine_transparent_fabric(node: Node) -> void:
    if node is MeshInstance3D:
        var mi := node as MeshInstance3D
        if str(mi.name).begins_with("Sheer curtain"):
            var material := StandardMaterial3D.new()
            material.albedo_color = Color(0.94, 0.91, 0.85, 0.22)
            material.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
            material.cull_mode = BaseMaterial3D.CULL_DISABLED
            material.roughness = 0.96
            material.backlight_enabled = true
            material.backlight = Color(0.40, 0.36, 0.30)
            mi.material_override = material
            mi.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
            mi.gi_mode = GeometryInstance3D.GI_MODE_DISABLED
        elif str(mi.name).begins_with("Window full height"):
            var material := StandardMaterial3D.new()
            material.albedo_color = Color(0.91, 0.95, 1.0, 0.045)
            material.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
            material.roughness = 0.08
            mi.material_override = material
            mi.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
            mi.gi_mode = GeometryInstance3D.GI_MODE_DISABLED
        elif mi.mesh != null:
            mi.gi_mode = GeometryInstance3D.GI_MODE_STATIC
            for surface_index in range(mi.mesh.get_surface_count()):
                var base := mi.get_active_material(surface_index) as BaseMaterial3D
                if base != null:
                    var clean := base.duplicate() as BaseMaterial3D
                    # Suppress tangent seams at packed atlas borders; retain actual color and roughness.
                    clean.normal_enabled = false
                    clean.albedo_texture = load("res://textures/baked/room_albedo_margin.png")
                    mi.set_surface_override_material(surface_index, clean)
            var label := str(mi.name)
            if label.begins_with("Rose outer curtain") or label.begins_with("Curtain tie band"):
                var cloth := StandardMaterial3D.new()
                cloth.albedo_color = Color(0.63, 0.40, 0.39)
                cloth.roughness = 0.96
                cloth.cull_mode = BaseMaterial3D.CULL_DISABLED
                cloth.normal_enabled = true
                cloth.normal_texture = load("res://textures/woven_fine_normal.png")
                cloth.normal_scale = 0.14
                cloth.uv1_triplanar = true
                cloth.uv1_world_triplanar = true
                cloth.uv1_scale = Vector3.ONE * 24.0
                cloth.backlight_enabled = true
                cloth.backlight = Color(0.10, 0.055, 0.045)
                mi.material_override = cloth
            elif label.begins_with("Left wall") or label.begins_with("Right wall") or label.begins_with("Ceiling."):
                var plaster := StandardMaterial3D.new()
                plaster.albedo_color = Color(0.82, 0.79, 0.73)
                plaster.roughness = 0.96
                mi.material_override = plaster
    for child in node.get_children():
        _refine_transparent_fabric(child)

# Attach depth and construction details to the existing city volumes.
# These remain actual 3D geometry, never an image pasted across the window.
func _city_detail_pass(room: Node3D) -> void:
    var buildings: Array[MeshInstance3D] = []
    for node in room.get_children():
        if node is MeshInstance3D and str(node.name).begins_with("Tokyo building"):
            buildings.append(node)
    var stone := StandardMaterial3D.new()
    stone.albedo_color = Color(0.54, 0.53, 0.50)
    stone.roughness = 0.9
    var dark_metal := StandardMaterial3D.new()
    dark_metal.albedo_color = Color(0.18, 0.20, 0.22)
    dark_metal.roughness = 0.65
    for index in range(buildings.size()):
        var building := buildings[index]
        var bounds: AABB = building.global_transform * building.get_aabb()
        var center := bounds.get_center()
        var roof := bounds.end.y
        var front := bounds.end.z
        # Cornice, recessed rooftop equipment, stepped side volume.
        _city_box(Vector3(center.x,roof+0.04,center.z),Vector3(bounds.size.x+0.06,0.08,bounds.size.z+0.05),stone)
        _city_box(Vector3(center.x-bounds.size.x*0.2,roof+0.20,center.z),Vector3(0.28,0.28,0.34),dark_metal)
        if index % 2 == 0:
            _city_box(Vector3(center.x+bounds.size.x*0.18,roof+0.36,center.z-0.14),Vector3(0.18,0.55,0.23),stone)
        var row: int = 1
        while bounds.position.y+float(row)*0.43 < roof-0.25:
            var level: float = bounds.position.y+float(row)*0.43
            # Projecting balcony slabs with thin front guards alternate by building.
            if index % 3 == 0:
                _city_box(Vector3(center.x,level-0.12,front+0.10),Vector3(bounds.size.x*0.92,0.035,0.24),stone)
                _city_box(Vector3(center.x,level+0.06,front+0.21),Vector3(bounds.size.x*0.90,0.18,0.018),dark_metal)
            else:
                _city_box(Vector3(center.x,level,front+0.018),Vector3(bounds.size.x*0.92,0.018,0.025),stone)
            row += 1
    # A modest elevated rail layer breaks up the repetitive facade band.
    _city_box(Vector3(-0.8,0.20,-11.0),Vector3(9.0,0.12,0.55),stone)
    for x in [-3.8,-1.3,1.2,3.7]:
        _city_box(Vector3(x,-0.85,-11.0),Vector3(0.18,2.0,0.30),stone)

func _city_box(position_world: Vector3, size: Vector3, material: Material) -> void:
    var node := MeshInstance3D.new()
    var box := BoxMesh.new()
    box.size = size
    box.material = material
    node.mesh = box
    node.position = position_world
    add_child(node)

func _balcony_plants() -> void:
    var leaf_material := StandardMaterial3D.new()
    leaf_material.albedo_color = Color(0.17, 0.30, 0.09)
    leaf_material.roughness = 0.75
    leaf_material.cull_mode = BaseMaterial3D.CULL_DISABLED
    var terracotta := StandardMaterial3D.new()
    terracotta.albedo_color = Color(0.64, 0.53, 0.41)
    terracotta.roughness = 0.9
    var soil := StandardMaterial3D.new()
    soil.albedo_color = Color(0.12,0.08,0.04)
    soil.roughness = 1.0
    var stem_material := StandardMaterial3D.new()
    stem_material.albedo_color = Color(0.18,0.23,0.08)
    stem_material.roughness = 0.9
    var leaf_arrays: Array = []
    leaf_arrays.resize(Mesh.ARRAY_MAX)
    leaf_arrays[Mesh.ARRAY_VERTEX] = PackedVector3Array([Vector3(0,0,0),Vector3(-0.055,0.055,0.008),Vector3(0,0.065,0.024),Vector3(0.055,0.055,0.008),Vector3(-0.032,0.13,-0.006),Vector3(0,0.14,0.016),Vector3(0.032,0.13,-0.006),Vector3(0,0.21,-0.012)])
    leaf_arrays[Mesh.ARRAY_INDEX] = PackedInt32Array([0,1,2,0,2,3,1,4,2,2,4,5,2,5,3,3,5,6,4,7,5,5,7,6])
    var leaf_mesh := ArrayMesh.new()
    leaf_mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES,leaf_arrays)
    # Generate smooth leaf normals, rather than flat color planes.
    var surface := SurfaceTool.new()
    surface.create_from(leaf_mesh,0)
    surface.generate_normals()
    leaf_mesh=surface.commit()
    leaf_mesh.surface_set_material(0,leaf_material)
    var rng := RandomNumberGenerator.new()
    rng.seed = 606
    for location in [Vector3(-1.00,-0.035,-5.05),Vector3(1.04,-0.035,-5.14)]:
        var planter := MeshInstance3D.new()
        var pot_mesh := CylinderMesh.new()
        pot_mesh.top_radius=.16;pot_mesh.bottom_radius=.12;pot_mesh.height=.27;pot_mesh.radial_segments=32
        pot_mesh.material=terracotta;planter.mesh=pot_mesh
        planter.position=location+Vector3(0,.135,0);add_child(planter)
        var dirt := MeshInstance3D.new()
        var dirt_mesh := CylinderMesh.new()
        dirt_mesh.top_radius=.146;dirt_mesh.bottom_radius=.146;dirt_mesh.height=.008;dirt_mesh.material=soil
        dirt.mesh=dirt_mesh;dirt.position=location+Vector3(0,.268,0);add_child(dirt)
        var leaves := MultiMeshInstance3D.new()
        var multimesh := MultiMesh.new()
        multimesh.transform_format=MultiMesh.TRANSFORM_3D;multimesh.mesh=leaf_mesh;multimesh.instance_count=72
        var count: int=0
        for branch in range(12):
            var angle: float=float(branch)*2.39996
            var height: float=rng.randf_range(.55,.95)
            var end: Vector3=location+Vector3(cos(angle)*.25,height,sin(angle)*.25)
            var start: Vector3=location+Vector3(0,.26,0)
            var stem := MeshInstance3D.new()
            var cylinder := CylinderMesh.new()
            cylinder.top_radius=.002;cylinder.bottom_radius=.005;cylinder.height=start.distance_to(end);cylinder.radial_segments=6;cylinder.material=stem_material
            stem.mesh=cylinder;stem.position=(start+end)*.5;add_child(stem)
            stem.quaternion=Quaternion(Vector3.UP,(end-start).normalized())
            for index in range(6):
                var position: Vector3=start.lerp(end,.35+float(index)*.12)
                var basis: Basis=Basis(Vector3.UP,angle+float(index)*2.4)*Basis(Vector3.RIGHT,rng.randf_range(.5,1.4))
                basis=basis.scaled(Vector3.ONE*rng.randf_range(.7,1.15))
                multimesh.set_instance_transform(count,Transform3D(basis,position))
                count+=1
        leaves.multimesh=multimesh;add_child(leaves)

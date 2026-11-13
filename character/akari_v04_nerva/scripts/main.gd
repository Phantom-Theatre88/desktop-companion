extends Node3D

# v0.3: Procedural idle + neutral standing. Godot Standard 4.3+ / GDScript.
const AVATAR_PATH := "res://avatar/akari_test.glb"
const NervaController = preload("res://scripts/nerva_controller.gd")
var _nerva = NervaController.new()
var expression_map: Dictionary = {}
var character: Node3D
var skeleton: Skeleton3D
var face: MeshInstance3D
var status_label: Label
var _mesh_count: int = 0
var _neck_idx: int = -1
var _head_idx: int = -1
var _left_eye_idx: int = -1
var _right_eye_idx: int = -1
var _yaw: float = 0.0
var _pitch: float = 0.0
var _eye_yaw: float = 0.0
var _smiling: bool = false
var _mouth_open: bool = false
var _blink_phase: float = -1.0
var _time_to_blink: float = 4.0
var _visibility_override: bool = false
var _original_materials: Dictionary = {}
var _diagnosis: String = ""
var _preview_placeholder: MeshInstance3D

# Only Character side generates the movement; Ghost will supply a compact state later.
var _idle_enabled: bool = true
var _arms_down: float = 0.94
var _idle_time: float = 0.0
var _gesture_energy: float = 0.35
var _target_head_yaw: float = 0.0
var _target_head_pitch: float = 0.0
var _target_eye_yaw: float = 0.0
var _target_body_yaw: float = 0.0
var _gaze_countdown: float = 1.2
var _rng := RandomNumberGenerator.new()
var _bones: Dictionary = {}
var _base_rot: Dictionary = {}
var _manual_yaw: float = 0.0
var _manual_pitch: float = 0.0
var _manual_eye_yaw: float = 0.0


func _ready() -> void:
    _nerva.initialize()
    _setup_ui()
    $Camera3D.position = Vector3(0.0, 1.43, 3.3)
    $Camera3D.look_at(Vector3(0.0, 1.05, 0.0), Vector3.UP)
    var map_file := FileAccess.open("res://avatar/vrm_expression_map.json", FileAccess.READ)
    if map_file != null:
        var decoded = JSON.parse_string(map_file.get_as_text())
        if decoded is Dictionary:
            expression_map = decoded
    _load_avatar()
    _update_status()

func _load_avatar() -> void:
    if not ResourceLoader.exists(AVATAR_PATH):
        _diagnosis = "ERROR: GLB resource missing. Check avatar/akari_test.glb"
        _show_placeholder()
        push_error(_diagnosis)
        return
    var loaded: Resource = ResourceLoader.load(AVATAR_PATH)
    if loaded == null or not (loaded is PackedScene):
        _diagnosis = "ERROR: Godot could not import GLB. Check Import dock / Output tab."
        _show_placeholder()
        push_error(_diagnosis)
        return
    var new_scene: Node = (loaded as PackedScene).instantiate()
    if not (new_scene is Node3D):
        _diagnosis = "ERROR: imported GLB root is not Node3D"
        _show_placeholder()
        push_error(_diagnosis)
        new_scene.queue_free()
        return
    character = new_scene as Node3D
    $AvatarAnchor.add_child(character)
    character.position = Vector3.ZERO
    character.scale = Vector3.ONE
    character.visible = true
    _find_components(character)
    _neck_idx = _bone_index("neck", "J_Bip_C_Neck")
    _head_idx = _bone_index("head", "J_Bip_C_Head")
    _left_eye_idx = _bone_index("leftEye", "J_Adj_L_FaceEye")
    _right_eye_idx = _bone_index("rightEye", "J_Adj_R_FaceEye")
    _cache_pose_bones()
    _apply_idle_pose(0.0)
    if _mesh_count == 0:
        _diagnosis = "WARNING: scene loaded but no MeshInstance3D found"
    else:
        _diagnosis = "GLB imported. If model invisible, try 'Show gray material' or 'Back view'."
    print("Akari v0.3: ", _diagnosis, " / meshes=", _mesh_count)

func _find_components(root: Node) -> void:
    if root is Skeleton3D and skeleton == null:
        skeleton = root as Skeleton3D
    if root is MeshInstance3D:
        _mesh_count += 1
        var mesh_node := root as MeshInstance3D
        mesh_node.visible = true
        if mesh_node.mesh != null and mesh_node.mesh.get_blend_shape_count() > 0:
            if face == null or str(mesh_node.name).to_lower().contains("face"):
                face = mesh_node
    for child in root.get_children():
        _find_components(child)

func _bone_index(vrm_name: String, fallback_name: String) -> int:
    if skeleton == null:
        return -1
    var mapping: Dictionary = expression_map.get("bones", {})
    return skeleton.find_bone(str(mapping.get(vrm_name, fallback_name)))

func _setup_ui() -> void:
    var canvas := CanvasLayer.new()
    add_child(canvas)
    var panel := PanelContainer.new()
    canvas.add_child(panel)
    panel.position = Vector2(18, 18)
    panel.custom_minimum_size = Vector2(340, 0)
    var layout := VBoxContainer.new()
    panel.add_child(layout)
    layout.add_child(_label("AKARI / autonomous idle test v0.4"))
    status_label = _label("Loading...")
    status_label.custom_minimum_size = Vector2(320, 120)
    layout.add_child(status_label)
    _button(layout, "Show gray material (diagnosis)", _toggle_materials)
    _button(layout, "Front / back view", _rotate_avatar)
    _button(layout, "Auto idle ON / OFF", func():
        _idle_enabled = not _idle_enabled
        if not _idle_enabled:
            _reset_targets()
            _apply_idle_pose(0.0)
        _update_status()
    )
    _slider(layout, "Arm relaxation", 0.0, 100.0, Callable(self, "_on_arm_relaxation_changed"), 94.0)
    _button(layout, "Blink now", func(): _nerva.request_blink())
    _button(layout, "Smile on / off", func():
        _smiling = not _smiling
        _apply_expression("happy", 1.0 if _smiling else 0.0)
    )
    _button(layout, "Mouth A on / off", func():
        _mouth_open = not _mouth_open
        _apply_expression("aa", 1.0 if _mouth_open else 0.0)
    )
    _slider(layout, "Neck Yaw", -35.0, 35.0, func(v: float):
        _manual_yaw = v
        _apply_idle_pose(0.0)
    )
    _slider(layout, "Neck Pitch", -20.0, 20.0, func(v: float):
        _manual_pitch = v
        _apply_idle_pose(0.0)
    )
    _slider(layout, "Eyes Yaw", -12.0, 12.0, func(v: float):
        _manual_eye_yaw = v
        _apply_idle_pose(0.0)
    )
    _button(layout, "Reset pose", func():
        _manual_yaw = 0.0
        _manual_pitch = 0.0
        _manual_eye_yaw = 0.0
        _reset_targets()
        _apply_idle_pose(0.0)
    )
    layout.add_child(_label("Ghost is not connected. This is a local test."))

func _label(value: String) -> Label:
    var result := Label.new()
    result.text = value
    result.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
    return result

func _button(layout: VBoxContainer, title: String, callback: Callable) -> void:
    var b := Button.new()
    b.text = title
    b.pressed.connect(callback)
    layout.add_child(b)

func _slider(layout: VBoxContainer, title: String, mn: float, mx: float, callback: Callable, initial: float = 0.0) -> void:
    layout.add_child(_label(title))
    var slider := HSlider.new()
    slider.min_value = mn
    slider.max_value = mx
    slider.step = 1.0
    slider.value = initial
    slider.custom_minimum_size.x = 290
    slider.value_changed.connect(callback)
    layout.add_child(slider)

func _show_placeholder() -> void:
    _preview_placeholder = MeshInstance3D.new()
    _preview_placeholder.name = "DEBUG_AVATAR_MISSING"
    var capsule := CapsuleMesh.new()
    capsule.radius = 0.22
    capsule.height = 1.5
    _preview_placeholder.mesh = capsule
    _preview_placeholder.position = Vector3(0.0, 0.82, 0.0)
    var m := StandardMaterial3D.new()
    m.albedo_color = Color(1.0, 0.3, 0.25)
    _preview_placeholder.material_override = m
    $AvatarAnchor.add_child(_preview_placeholder)

func _toggle_materials() -> void:
    if character == null:
        return
    _visibility_override = not _visibility_override
    _apply_visibility_to_tree(character)
    _update_status()

func _apply_visibility_to_tree(node: Node) -> void:
    if node is MeshInstance3D:
        var mesh_node := node as MeshInstance3D
        var id := mesh_node.get_instance_id()
        if not _original_materials.has(id):
            _original_materials[id] = mesh_node.material_override
        if _visibility_override:
            var mat := StandardMaterial3D.new()
            mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
            mat.albedo_color = Color(0.75, 0.82, 0.89)
            mat.cull_mode = BaseMaterial3D.CULL_DISABLED
            mat.transparency = BaseMaterial3D.TRANSPARENCY_DISABLED
            mesh_node.material_override = mat
        else:
            mesh_node.material_override = _original_materials[id]
    for child in node.get_children():
        _apply_visibility_to_tree(child)

func _rotate_avatar() -> void:
    if character != null:
        character.rotation.y += PI

func _expression_shape(name: String) -> String:
    var binds: Array = expression_map.get("expressions", {}).get(name, [])
    if binds.is_empty():
        return ""
    return str(binds[0].get("blend_shape", ""))

func _apply_expression(name: String, value: float) -> void:
    if face == null or face.mesh == null:
        return
    var name_to_match := _expression_shape(name)
    for i in range(face.mesh.get_blend_shape_count()):
        if str(face.mesh.get_blend_shape_name(i)) == name_to_match:
            face.set_blend_shape_value(i, clampf(value, 0.0, 1.0))
            return

func _on_arm_relaxation_changed(value: float) -> void:
    _arms_down = value / 100.0
    _apply_idle_pose(0.0)

# Pose changes are applied as a rotation offset on top of the imported rest pose.
func _cache_pose_bones() -> void:
    _nerva.bind_model(skeleton, expression_map)

func _sync_nerva() -> void:
    _nerva._idle_enabled = _idle_enabled
    _nerva._arms_down = _arms_down
    _nerva._gesture_energy = _gesture_energy
    _nerva._manual_yaw = _manual_yaw
    _nerva._manual_pitch = _manual_pitch
    _nerva._manual_eye_yaw = _manual_eye_yaw

func _reset_targets() -> void:
    _nerva._reset_targets()

func _apply_idle_pose(delta: float) -> void:
    _sync_nerva()
    _nerva._apply_idle_pose(delta)

func _process(delta: float) -> void:
    _sync_nerva()
    _nerva.step(delta)
    _apply_expression("blink", _nerva.blink_value)

func _update_status() -> void:
    if status_label == null:
        return
    var lines: Array[String] = []
    lines.append("Meshes: %s" % str(_mesh_count))
    lines.append("Skeleton: %s" % ("OK" if skeleton != null else "MISSING"))
    lines.append("Face morphs: %s" % (str(face.mesh.get_blend_shape_count()) if face != null and face.mesh != null else "MISSING"))
    lines.append("Neck: %s" % ("OK" if _neck_idx >= 0 else "MISSING"))
    lines.append("Idle: %s" % ("ON" if _idle_enabled else "OFF"))
    lines.append("Pose bones: %s" % str(_nerva._bones.size()))
    lines.append("Material override: %s" % ("ON" if _visibility_override else "OFF"))
    lines.append(_diagnosis)
    status_label.text = "\n".join(lines)


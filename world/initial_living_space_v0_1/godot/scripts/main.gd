extends Node3D

# v0.3: Procedural idle + neutral standing. Godot Standard 4.3+ / GDScript.
const AVATAR_PATH := "res://avatar/akari_test.glb"
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
    _rng.randomize()
    _time_to_blink = _rng.randf_range(2.7, 5.2)
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
    _button(layout, "Blink now", func(): _blink_phase = 0.0)
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
# Cache initial bone poses once so the animation never accumulates drift.
func _cache_pose_bones() -> void:
    if skeleton == null:
        return
    var names: Array[String] = [
        "hips", "spine", "chest", "upperChest", "neck", "head",
        "leftShoulder", "rightShoulder", "leftUpperArm", "rightUpperArm",
        "leftLowerArm", "rightLowerArm", "leftEye", "rightEye"
    ]
    for n in names:
        var idx := _bone_index(n, "")
        if idx >= 0:
            _bones[n] = idx
            _base_rot[n] = skeleton.get_bone_pose_rotation(idx)

func _rot(name: String, offset: Quaternion) -> void:
    if skeleton == null or not _bones.has(name):
        return
    var base: Quaternion = _base_rot[name]
    skeleton.set_bone_pose_rotation(int(_bones[name]), (base * offset).normalized())

func _reset_targets() -> void:
    _target_head_yaw = 0.0
    _target_head_pitch = 0.0
    _target_eye_yaw = 0.0
    _target_body_yaw = 0.0
    _yaw = 0.0
    _pitch = 0.0
    _eye_yaw = 0.0

func _apply_idle_pose(_delta: float) -> void:
    if skeleton == null:
        return
    var t := _idle_time
    var enable := 1.0 if _idle_enabled else 0.0
    var energy := _gesture_energy * enable
    var breath := sin(t * 1.65)
    var sway := sin(t * 0.41) * 0.7 + sin(t * 0.67 + 0.3) * 0.3
    var ease := 1.0 - pow(1.0 - clampf(_arms_down, 0.0, 1.0), 2.0)

    # Rest model is a T-pose. Lower the arms toward the body with a soft, subtle swing.
    # v0.4: invert the prior shoulder-roll sign. v0.3 raised both hands overhead
    # in the actual Godot runtime; these offsets aim downward instead.
    var left_drop := deg_to_rad(72.0 * ease + energy * sin(t * 0.63) * 2.5)
    var right_drop := deg_to_rad(-72.0 * ease - energy * sin(t * 0.57 + 1.3) * 2.5)
    _rot("leftUpperArm", Quaternion(Vector3.FORWARD, left_drop))
    _rot("rightUpperArm", Quaternion(Vector3.FORWARD, right_drop))
    _rot("leftLowerArm", Quaternion(Vector3.FORWARD, deg_to_rad(-4.0 * ease)))
    _rot("rightLowerArm", Quaternion(Vector3.FORWARD, deg_to_rad(4.0 * ease)))

    # Gentle body movement. Do not animate hips translation or feet: feet stay planted.
    _rot("spine", Quaternion(Vector3.FORWARD, deg_to_rad(energy * sway * 0.65)))
    _rot("chest", Quaternion(Vector3.RIGHT, deg_to_rad(energy * breath * 0.85)))
    _rot("upperChest", Quaternion(Vector3.UP, deg_to_rad(energy * _target_body_yaw * 0.35)))

    var y := _manual_yaw + _yaw + energy * sin(t * 0.31) * 1.1
    var p := _manual_pitch + _pitch + energy * sin(t * 0.28 + 0.7) * 0.8
    var head_rot := Quaternion(Vector3.UP, deg_to_rad(y)) * Quaternion(Vector3.RIGHT, deg_to_rad(p))
    _rot("neck", head_rot)
    _rot("head", Quaternion(Vector3.FORWARD, deg_to_rad(energy * sin(t * 0.23 + 1.2) * 1.5)))
    var eye_rot := Quaternion(Vector3.UP, deg_to_rad(_manual_eye_yaw + _eye_yaw))
    _rot("leftEye", eye_rot)
    _rot("rightEye", eye_rot)

func _process(delta: float) -> void:
    if _idle_enabled:
        _idle_time += delta
        _gaze_countdown -= delta
        if _gaze_countdown <= 0.0:
            # New targets at irregular intervals; no fixed repeating animation clip.
            _gaze_countdown = _rng.randf_range(1.9, 5.7)
            _target_head_yaw = _rng.randf_range(-10.0, 10.0)
            _target_head_pitch = _rng.randf_range(-5.0, 5.0)
            _target_eye_yaw = _rng.randf_range(-8.0, 8.0)
            _target_body_yaw = _rng.randf_range(-4.0, 4.0)
        var weight := 1.0 - exp(-delta * 1.7)
        _yaw = lerpf(_yaw, _target_head_yaw, weight)
        _pitch = lerpf(_pitch, _target_head_pitch, weight)
        _eye_yaw = lerpf(_eye_yaw, _target_eye_yaw, 1.0 - exp(-delta * 3.0))
    _apply_idle_pose(delta)

    if _blink_phase >= 0.0:
        _blink_phase += delta
        var amount := 1.0 - absf(_blink_phase - 0.105) / 0.105
        _apply_expression("blink", maxf(0.0, amount))
        if _blink_phase >= 0.21:
            _blink_phase = -1.0
            _apply_expression("blink", 0.0)
    elif _idle_enabled:
        _time_to_blink -= delta
        if _time_to_blink <= 0.0:
            _time_to_blink = _rng.randf_range(2.5, 6.2)
            _blink_phase = 0.0

func _update_status() -> void:
    if status_label == null:
        return
    var lines: Array[String] = []
    lines.append("Meshes: %s" % str(_mesh_count))
    lines.append("Skeleton: %s" % ("OK" if skeleton != null else "MISSING"))
    lines.append("Face morphs: %s" % (str(face.mesh.get_blend_shape_count()) if face != null and face.mesh != null else "MISSING"))
    lines.append("Neck: %s" % ("OK" if _neck_idx >= 0 else "MISSING"))
    lines.append("Idle: %s" % ("ON" if _idle_enabled else "OFF"))
    lines.append("Pose bones: %s" % str(_bones.size()))
    lines.append("Material override: %s" % ("ON" if _visibility_override else "OFF"))
    lines.append(_diagnosis)
    status_label.text = "\n".join(lines)

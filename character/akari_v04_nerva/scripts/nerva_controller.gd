extends RefCounted
## NERVA movement controller extracted from the ACTUAL Dropbox v0.4 main.gd.
## Movement curves, arm rotation signs and clamp limits are intentionally retained.
## Ghost is not required. No emotions or personal intention are generated here.
var skeleton: Skeleton3D
var expression_map: Dictionary = {}
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
var _yaw: float = 0.0
var _pitch: float = 0.0
var _eye_yaw: float = 0.0
var _blink_phase: float = -1.0
var _time_to_blink: float = 4.0
var blink_value: float = 0.0

func initialize() -> void:
    _rng.randomize()
    _time_to_blink = _rng.randf_range(2.7, 5.2)

func bind_model(model_skeleton: Skeleton3D, model_expression_map: Dictionary) -> void:
    skeleton = model_skeleton
    expression_map = model_expression_map
    _bones.clear()
    _base_rot.clear()
    _cache_pose_bones()

func _bone_index(vrm_name: String, fallback_name: String) -> int:
    if skeleton == null:
        return -1
    var mapping: Dictionary = expression_map.get("bones", {})
    return skeleton.find_bone(str(mapping.get(vrm_name, fallback_name)))

func request_blink() -> void:
    _blink_phase = 0.0

# The following movement calculations are copied from Character v0.4.
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

func step(delta: float) -> void:
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
        blink_value = maxf(0.0, amount)
        if _blink_phase >= 0.21:
            _blink_phase = -1.0
            blink_value = 0.0
    elif _idle_enabled:
        _time_to_blink -= delta
        if _time_to_blink <= 0.0:
            _time_to_blink = _rng.randf_range(2.5, 6.2)
            _blink_phase = 0.0



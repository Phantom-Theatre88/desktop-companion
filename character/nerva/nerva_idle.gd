extends Node3D
## NERVA v0.1 — procedural autonomous idle motor controller.
## No Ghost state, emotions, or intentions are invented here.
## Character keeps breathing, blinking, and small posture/gaze shifts
## when Ghost is absent. Never plays numbered, baked motion clips.

@export var skeleton_path: NodePath
@export var face_mesh_path: NodePath
@export var head_bone_name: String = "Head"
@export var neck_bone_name: String = "Neck"
@export var chest_bone_name: String = "Chest"
@export var left_shoulder_bone_name: String = "LeftShoulder"
@export var right_shoulder_bone_name: String = "RightShoulder"
@export var left_eye_bone_name: String = "LeftEye"
@export var right_eye_bone_name: String = "RightEye"
@export var blink_shape_name: String = "Blink"
@export_range(0.0, 1.0, 0.01) var intensity: float = 0.65
@export var random_seed: int = 0

var _skeleton: Skeleton3D
var _face_mesh: MeshInstance3D
var _bone_indices: Dictionary = {}
var _rest_pose_rotations: Dictionary = {}
var _rng: RandomNumberGenerator = RandomNumberGenerator.new()
var _elapsed: float = 0.0
var _target_elapsed: float = 0.0
var _target_interval: float = 3.5
var _gaze_target: Vector2 = Vector2.ZERO
var _gaze_current: Vector2 = Vector2.ZERO
var _head_target: Vector2 = Vector2.ZERO
var _head_current: Vector2 = Vector2.ZERO
var _posture_target: float = 0.0
var _posture_current: float = 0.0
var _blink_next: float = 2.7
var _blink_remaining: float = 0.0
var _blink_duration: float = 0.16
var _blink_shape_index: int = -1
var _blink_amount: float = 0.0
var _has_binding: bool = false

func _ready() -> void:
	if random_seed != 0:
		_rng.seed = random_seed
	else:
		_rng.randomize()
	_resolve_binding()
	_schedule_next_target()
	_schedule_blink()
	set_process(true)

func _resolve_binding() -> void:
	_skeleton = get_node_or_null(skeleton_path) as Skeleton3D
	_face_mesh = get_node_or_null(face_mesh_path) as MeshInstance3D
	_bone_indices.clear()
	_rest_pose_rotations.clear()
	if _skeleton == null:
		push_warning("NERVA idle: Skeleton3D not assigned. Controller stays ready but does not modify a body.")
		return
	var targets := {
		"head": head_bone_name,
		"neck": neck_bone_name,
		"chest": chest_bone_name,
		"left_shoulder": left_shoulder_bone_name,
		"right_shoulder": right_shoulder_bone_name,
		"left_eye": left_eye_bone_name,
		"right_eye": right_eye_bone_name,
	}
	for key in targets:
		var index := _skeleton.find_bone(str(targets[key]))
		if index >= 0:
			_bone_indices[key] = index
			_rest_pose_rotations[key] = _skeleton.get_bone_pose_rotation(index)
	if _face_mesh != null and _face_mesh.mesh != null:
		for shape_index in range(_face_mesh.mesh.get_blend_shape_count()):
			if str(_face_mesh.mesh.get_blend_shape_name(shape_index)) == blink_shape_name:
				_blink_shape_index = shape_index
				break
	_has_binding = _bone_indices.size() > 0 or _blink_shape_index >= 0

func _schedule_next_target() -> void:
	_target_elapsed = 0.0
	_target_interval = _rng.randf_range(2.2, 6.8)
	_gaze_target = Vector2(_rng.randf_range(-0.72, 0.72), _rng.randf_range(-0.34, 0.30))
	_head_target = Vector2(_rng.randf_range(-0.32, 0.32), _rng.randf_range(-0.20, 0.18))
	_posture_target = _rng.randf_range(-0.75, 0.75)

func _schedule_blink() -> void:
	_blink_next = _elapsed + _rng.randf_range(2.3, 7.2)

func _process(delta: float) -> void:
	step_idle(delta)

func step_idle(delta: float) -> void:
	# Public for deterministic tests; WORLD preview time must not drive this clock.
	if delta <= 0.0:
		return
	var dt := minf(delta, 0.10)
	_elapsed += dt
	_target_elapsed += dt
	if _target_elapsed >= _target_interval:
		_schedule_next_target()
	# Continuous response rather than switching among animation IDs.
	var gaze_alpha := 1.0 - exp(-dt * 3.0)
	var head_alpha := 1.0 - exp(-dt * 1.25)
	var posture_alpha := 1.0 - exp(-dt * 0.65)
	_gaze_current = _gaze_current.lerp(_gaze_target, gaze_alpha)
	_head_current = _head_current.lerp(_head_target, head_alpha)
	_posture_current = lerpf(_posture_current, _posture_target, posture_alpha)
	_update_blink(dt)
	_apply_bones()

func _update_blink(dt: float) -> void:
	if _blink_remaining <= 0.0 and _elapsed >= _blink_next:
		_blink_duration = _rng.randf_range(0.13, 0.22)
		_blink_remaining = _blink_duration
		_schedule_blink()
	if _blink_remaining > 0.0:
		var phase := 1.0 - _blink_remaining / _blink_duration
		_blink_amount = pow(maxf(0.0, sin(phase * PI)), 0.7)
		_blink_remaining = maxf(0.0, _blink_remaining - dt)
	else:
		_blink_amount = 0.0
	if _face_mesh != null and _blink_shape_index >= 0:
		_face_mesh.set_blend_shape_value(_blink_shape_index, _blink_amount)

func _apply_bones() -> void:
	if _skeleton == null:
		return
	var level := clampf(intensity, 0.0, 1.0)
	var breath := sin(_elapsed * TAU / 4.3) * 0.018 * level
	var sway := sin(_elapsed * TAU / 8.1 + 0.4) * 0.010 * level
	var head_angles := Vector3(
		_head_current.y * 0.12 + breath * 0.20,
		_head_current.x * 0.18,
		sway + _posture_current * 0.018
	) * level
	var neck_angles := head_angles * 0.38
	var chest_angles := Vector3(breath, sway * 0.45, _posture_current * 0.013) * level
	_apply_rotation("head", head_angles)
	_apply_rotation("neck", neck_angles)
	_apply_rotation("chest", chest_angles)
	_apply_rotation("left_shoulder", Vector3(breath * 0.25, 0, sway * 0.35))
	_apply_rotation("right_shoulder", Vector3(breath * 0.25, 0, -sway * 0.35))
	var eye_angles := Vector3(_gaze_current.y * 0.18, _gaze_current.x * 0.20, 0.0) * level
	_apply_rotation("left_eye", eye_angles)
	_apply_rotation("right_eye", eye_angles)

func _apply_rotation(key: String, extra_euler: Vector3) -> void:
	if not _bone_indices.has(key):
		return
	var index: int = _bone_indices[key]
	var base: Quaternion = _rest_pose_rotations[key]
	var offset := Quaternion.from_euler(extra_euler)
	_skeleton.set_bone_pose_rotation(index, (base * offset).normalized())

func diagnostics() -> Dictionary:
	return {
		"bound": _has_binding,
		"elapsed": _elapsed,
		"blink": _blink_amount,
		"gaze": _gaze_current,
		"head": _head_current,
		"posture": _posture_current,
		"bone_count": _bone_indices.size()
	}

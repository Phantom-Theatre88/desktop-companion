extends RefCounted
## NERVA movement controller extracted from the ACTUAL Dropbox v0.4 main.gd.
## v0.5: relaxed hands and coordinated autonomous idle; no Ghost intentions generated.
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
var _ghost_intent: String = "idle"
var _ghost_strength: float = 0.0
var _ghost_intent_remaining: float = 0.0
var _ghost_state: Dictionary = {}
# Motor expression, not Ghost personality. These phases are continuous, not clips.
# Fixed behavior semantics: idle=relaxed gesture, scan=attentive gesture,
# drowsy=stretch/yawn posture, sleep=quiet breathing.
var _body_action_time: float = 0.0
var _body_action_index: int = 0
var _body_action_name: String = "settle"
var _last_intent: String = "idle"
var _gesture_timer: float = 0.0
var _gesture_period: float = 8.0
var _gesture_blend: float = 0.0
var _gesture_side: float = 1.0
var visible_gestures: int = 0


func accept_ghost_state(state: Dictionary) -> void:
    # Read only the six existing Heart compatibility values. NERVA never infers emotions.
    _ghost_state = state.duplicate()

func clear_ghost_state() -> void:
    _ghost_state.clear()

func accept_ghost_intent(kind: String, strength: float) -> void:
    if not kind in ["idle", "attend", "rest", "blink", "scan", "drowsy", "sleep"]:
        return
    print("NERVA received Ghost intent: ", kind)
    if kind != _ghost_intent:
        _body_action_time = 0.0
        _gesture_timer = 0.0
    _ghost_intent = kind
    _ghost_strength = clampf(strength, 0.0, 1.0)
    _ghost_intent_remaining = -1.0 if kind in ["idle", "scan", "drowsy", "sleep"] else 3.0
    if kind == "attend":
        _focus_yaw = 0.0
        _focus_pitch = -5.0 * _ghost_strength
        _target_head_yaw = 0.0
        _target_head_pitch = _focus_pitch * 0.6
        _head_delay = 0.15
        _gaze_countdown = 3.0
    elif kind == "scan":
        _gaze_countdown = 0.0
    elif kind == "sleep":
        _reset_targets()
    elif kind == "blink":
        request_blink()

func clear_ghost_intent() -> void:
    _ghost_intent = "idle"
    _ghost_strength = 0.0
    _ghost_intent_remaining = 0.0
var _breath_phase: float = 0.0
var _breath_period: float = 4.4
var _breath_depth: float = 1.0
var _focus_yaw: float = 0.0
var _focus_pitch: float = 0.0
var _eye_pitch: float = 0.0
var _head_delay: float = 0.0
var _double_blink_pending: bool = false
var _double_blink_delay: float = -1.0
var focus_name: String = "forward"
var focus_changes: int = 0
var breath_cycles: int = 0
var blink_count: int = 0
# Local idle attention directions, not WORLD perception or Ghost decisions.
var attention_points: Array[Dictionary] = [
    {"name": "forward", "yaw": 0.0, "pitch": 0.0},
    {"name": "left_near", "yaw": -14.0, "pitch": 4.0},
    {"name": "right_near", "yaw": 14.0, "pitch": 3.0},
    {"name": "up_near", "yaw": 4.0, "pitch": -7.0},
    {"name": "down_near", "yaw": -5.0, "pitch": 8.0}
]
var _last_focus: int = -1
# 2026-11-16 restoration: gentle stance transfer only, NO walking/IK/foot lock.
var _stance_enabled: bool = true
var _stance_blend: float = 0.0
var _stance_target: float = 0.0
var _stance_next: float = 0.0
var stance_transfers: int = 0
var _last_stance_delta: float = 0.0

func initialize() -> void:
    _rng.randomize()
    _time_to_blink = _rng.randf_range(2.7, 5.2)
    _gesture_period = 6.0
    _breath_period = _rng.randf_range(3.8, 5.1)

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
    blink_count += 1

# Bind rest poses once, including wrists and finger joints.
func _cache_pose_bones() -> void:
    if skeleton == null:
        return
    var names: Array[String] = [
        "hips", "spine", "chest", "upperChest", "neck", "head",
        "leftShoulder", "rightShoulder", "leftUpperArm", "rightUpperArm",
        "leftLowerArm", "rightLowerArm", "leftEye", "rightEye",
        "leftUpperLeg", "leftLowerLeg", "leftFoot", "rightUpperLeg", "rightLowerLeg", "rightFoot"
    ]
    names.append_array(["leftHand", "rightHand"])
    for side in ["left", "right"]:
        for finger in ["Index", "Middle", "Ring", "Little"]:
            for segment in ["Proximal", "Intermediate", "Distal"]:
                names.append(side + finger + segment)
        for segment in ["Metacarpal", "Proximal", "Distal"]:
            names.append(side + "Thumb" + segment)
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
    _eye_pitch = 0.0
    _focus_yaw = 0.0
    _focus_pitch = 0.0
    _head_delay = 0.0
    _gaze_countdown = 1.2

func _apply_idle_pose(_delta: float) -> void:
    if skeleton == null:
        return
    var t := _idle_time
    var enable := 1.0 if _idle_enabled else 0.0
    var energy := _gesture_energy * enable
    if _ghost_intent in ["rest", "drowsy"]:
        energy *= 1.0 - 0.8 * _ghost_strength
    elif _ghost_intent == "sleep":
        energy = 0.0
    var breath := sin(_breath_phase) * _breath_depth
    var sway := sin(t * 0.41) * 0.7 + sin(t * 0.67 + 0.3) * 0.3
    var ease := 1.0 - pow(1.0 - clampf(_arms_down, 0.0, 1.0), 2.0)

    # Rest model is a T-pose. Lower the arms toward the body with a soft, subtle swing.
    # v0.4: invert the prior shoulder-roll sign. v0.3 raised both hands overhead
    # in the actual Godot runtime; these offsets aim downward instead.
    var left_drop := deg_to_rad(79.0 * ease + energy * sin(t * 0.63) * 2.5)
    var right_drop := deg_to_rad(-79.0 * ease - energy * sin(t * 0.57 + 1.3) * 2.5)
    # Express semantic Ghost intent as a continuous NERVA body movement.
    # A soft envelope prevents joints snapping between poses.
    var g: float = _gesture_blend
    var left_extra: float = 0.0
    var right_extra: float = 0.0
    var left_elbow: float = 0.0
    var right_elbow: float = 0.0
    var body_lean: float = 0.0
    if _ghost_intent == "sleep":
        _body_action_name = "sleep breathing"
    elif _ghost_intent == "drowsy":
        _body_action_name = "drowsy stretch"
        # Bring arms forward and slightly out, never fake walking or seating.
        left_extra = -34.0 * g
        right_extra = 34.0 * g
        left_elbow = -32.0 * g
        right_elbow = 32.0 * g
        body_lean = -4.0 * g
    elif _ghost_intent == "scan":
        _body_action_name = "attentive gesture"
        left_extra = -13.0 * g
        right_extra = 31.0 * g
        right_elbow = 39.0 * g
        body_lean = 3.0 * g
    elif _ghost_intent in ["idle", "attend"]:
        _body_action_name = "relaxed spontaneous gesture" if g > 0.12 else "settle"
        if _gesture_side > 0.0:
            right_extra = 38.0 * g
            right_elbow = 48.0 * g
        else:
            left_extra = -38.0 * g
            left_elbow = -48.0 * g
        body_lean = 3.5 * g * _gesture_side
    # Existing T-pose correction is preserved; only upper body is composed.
    _rot("leftUpperArm", Quaternion(Vector3.FORWARD, left_drop + deg_to_rad(left_extra)) * Quaternion(Vector3.RIGHT, deg_to_rad(13.0 * g)))
    _rot("rightUpperArm", Quaternion(Vector3.FORWARD, right_drop + deg_to_rad(right_extra)) * Quaternion(Vector3.RIGHT, deg_to_rad(-13.0 * g)))

    _rot("leftLowerArm", Quaternion(Vector3.UP, deg_to_rad(-7.0 * ease + left_elbow)))
    _rot("rightLowerArm", Quaternion(Vector3.UP, deg_to_rad(7.0 * ease + right_elbow)))
    _apply_relaxed_hands(t, enable, ease)

    # Gentle body movement. Do not animate hips translation or feet: feet stay planted.
    _rot("spine", Quaternion(Vector3.FORWARD, deg_to_rad(energy * sway * 0.65 + body_lean)))
    # Breathing remains visible even during sleep.
    _rot("chest", Quaternion(Vector3.RIGHT, deg_to_rad(_gesture_energy * enable * breath * 0.85)))
    _rot("upperChest", Quaternion(Vector3.UP, deg_to_rad(energy * _target_body_yaw * 0.35)))
    _apply_stance(t, enable, energy)

    var y := _manual_yaw + _yaw + energy * sin(t * 0.31) * 1.1
    var p := _manual_pitch + _pitch + energy * sin(t * 0.28 + 0.7) * 0.8
    var head_rot := Quaternion(Vector3.UP, deg_to_rad(y)) * Quaternion(Vector3.RIGHT, deg_to_rad(p))
    _rot("neck", head_rot)
    _rot("head", Quaternion(Vector3.FORWARD, deg_to_rad(energy * sin(t * 0.23 + 1.2) * 1.5)))
    var eye_rot := Quaternion(Vector3.UP, deg_to_rad(clampf(_manual_eye_yaw + _eye_yaw, -18.0, 18.0))) * Quaternion(Vector3.RIGHT, deg_to_rad(_eye_pitch))
    _rot("leftEye", eye_rot)
    _rot("rightEye", eye_rot)

func _apply_stance(t: float, enable: float, energy: float) -> void:
    # Conservative, smooth joint rotations around the rest pose. Not IK.
    if not _stance_enabled or not _bones.has("leftUpperLeg") or not _bones.has("rightUpperLeg"):
        return
    var awake: float = 0.0 if _ghost_intent == "sleep" else 1.0
    var target: float = _stance_target * awake * enable
    _stance_blend = lerpf(_stance_blend, target, 1.0 - exp(-0.8 * maxf(0.0, _last_stance_delta)))
    # November 16 recorded boredom/sleepiness dependence. Direct continuous
    # attenuation only: no newly designed emotional thresholds or selection policy.
    var boredom: float = float(_ghost_state.get("boredom", 0.15))
    var sleepiness: float = float(_ghost_state.get("sleepiness", 0.15))
    var state_factor: float = clampf(boredom * (1.0 - sleepiness), 0.0, 1.0)
    var level: float = _stance_blend * minf(0.4, energy + 0.05) * state_factor
    _rot("leftUpperLeg", Quaternion(Vector3.FORWARD, deg_to_rad(level * 1.8)))
    _rot("rightUpperLeg", Quaternion(Vector3.FORWARD, deg_to_rad(-level * 1.8)))
    _rot("leftLowerLeg", Quaternion(Vector3.RIGHT, deg_to_rad(absf(level) * 0.65)))
    _rot("rightLowerLeg", Quaternion(Vector3.RIGHT, deg_to_rad(absf(level) * 0.65)))
    _rot("leftFoot", Quaternion(Vector3.RIGHT, deg_to_rad(-absf(level) * 0.55)))
    _rot("rightFoot", Quaternion(Vector3.RIGHT, deg_to_rad(-absf(level) * 0.55)))

func _apply_relaxed_hands(t: float, enable: float, ease: float) -> void:
    for side in ["left", "right"]:
        var sign_side: float = 1.0 if side == "left" else -1.0
        var phase: float = 0.0 if side == "left" else 1.6
        var soften: float = sin(t * 0.38 + phase) * 1.2 * enable
        _rot(side + "Hand", Quaternion(Vector3.FORWARD, deg_to_rad(sign_side * (4.0 * ease + soften))))
        var index: int = 0
        for finger in ["Index", "Middle", "Ring", "Little"]:
            # VRoid model: fingers extend along +/-X; curl toward palm (-Y).
            # Unequal, conservative bends avoid a clenched fist or rigid flat hand.
            var curl: float = (12.0 + float(index) * 3.0 + soften) * ease
            _rot(side + finger + "Proximal", Quaternion(Vector3.FORWARD, deg_to_rad(sign_side * curl)))
            _rot(side + finger + "Intermediate", Quaternion(Vector3.FORWARD, deg_to_rad(sign_side * curl * 0.75)))
            _rot(side + finger + "Distal", Quaternion(Vector3.FORWARD, deg_to_rad(sign_side * curl * 0.4)))
            index += 1
        # Thumb rest axes differ from the other fingers; retain its spread.
        _rot(side + "ThumbProximal", Quaternion(Vector3.FORWARD, deg_to_rad(sign_side * 6.0 * ease)))
        _rot(side + "ThumbDistal", Quaternion(Vector3.FORWARD, deg_to_rad(sign_side * 8.0 * ease)))

func _choose_attention() -> void:
    var selected: int = _rng.randi_range(0, attention_points.size() - 2)
    if selected >= _last_focus:
        selected += 1
    selected = clampi(selected, 0, attention_points.size() - 1)
    _last_focus = selected
    var point: Dictionary = attention_points[selected]
    focus_name = point.name
    focus_changes += 1
    _focus_yaw = float(point.yaw) + _rng.randf_range(-1.5, 1.5)
    _focus_pitch = float(point.pitch) + _rng.randf_range(-1.0, 1.0)
    _target_head_yaw = _focus_yaw * 0.65
    _target_head_pitch = _focus_pitch * 0.6
    _target_body_yaw = _focus_yaw * 0.12
    _head_delay = _rng.randf_range(0.15, 0.32)
    _gaze_countdown = _rng.randf_range(2.8, 6.8)
    if _rng.randf() < 0.3 and _blink_phase < 0.0:
        _time_to_blink = minf(_time_to_blink, 0.12)

func step(delta: float) -> void:
    _body_action_time += delta
    _gesture_timer += delta
    # Only NERVA's motor variation; Ghost still owns the action category.
    # Vary motion timing without adding arbitrary emotion or personality scores.
    if _gesture_timer >= _gesture_period:
        _gesture_timer = 0.0
        _gesture_period = _rng.randf_range(7.5, 12.0)
        _body_action_index += 1
        _gesture_side = -_gesture_side
        visible_gestures += 1
    var gesture_goal: float = 0.0
    if _ghost_intent == "scan":
        gesture_goal = 0.7 + 0.2 * sin(_body_action_time * 0.65)
    elif _ghost_intent == "drowsy":
        gesture_goal = 0.8 + 0.1 * sin(_body_action_time * 0.45)
    elif _ghost_intent == "attend":
        gesture_goal = 0.75
    elif _ghost_intent == "idle" and _gesture_timer < 3.2:
        # A short gesture then relaxed waiting. Not constant arm waving.
        gesture_goal = sin(PI * _gesture_timer / 3.2)
    _gesture_blend = lerpf(_gesture_blend, gesture_goal, 1.0 - exp(-maxf(delta, 0.0) * 2.5))
    _last_stance_delta = delta
    _stance_next -= delta
    if _stance_next <= 0.0:
        _stance_next = _rng.randf_range(6.0, 12.0)
        _stance_target = _rng.randf_range(-1.0, 1.0)
        stance_transfers += 1
    if _ghost_intent_remaining > 0.0:
        _ghost_intent_remaining -= delta
        if _ghost_intent_remaining <= 0.0:
            clear_ghost_intent()
    if _idle_enabled:
        _idle_time += delta
        _breath_phase += delta * TAU / _breath_period
        if _breath_phase >= TAU:
            _breath_phase = fmod(_breath_phase, TAU)
            breath_cycles += 1
            _breath_period = _rng.randf_range(3.8, 5.1)
            _breath_depth = _rng.randf_range(0.85, 1.15)
        _gaze_countdown -= delta
        if _gaze_countdown <= 0.0 and _ghost_intent != "sleep":
            _choose_attention()
        _head_delay = maxf(0.0, _head_delay - delta)
        if _head_delay <= 0.0:
            var weight := 1.0 - exp(-delta * 1.7)
            _yaw = lerpf(_yaw, _target_head_yaw, weight)
            _pitch = lerpf(_pitch, _target_head_pitch, weight)
        # Eyes lead; the neck follows, leaving only the residual eye angle.
        _eye_yaw = lerpf(_eye_yaw, clampf(_focus_yaw - _yaw, -12.0, 12.0), 1.0 - exp(-delta * 9.0))
        _eye_pitch = lerpf(_eye_pitch, clampf(_focus_pitch - _pitch, -8.0, 8.0), 1.0 - exp(-delta * 9.0))
    _apply_idle_pose(delta)
    if _ghost_intent == "sleep":
        blink_value = 1.0
        return
    if _blink_phase >= 0.0:
        _blink_phase += delta
        # Fast closure, slightly slower reopening, with smooth endpoints.
        if _blink_phase < 0.075:
            blink_value = smoothstep(0.0, 0.075, _blink_phase)
        else:
            blink_value = 1.0 - smoothstep(0.075, 0.23, _blink_phase)
        if _blink_phase >= 0.23:
            _blink_phase = -1.0
            blink_value = 0.0
            if _double_blink_pending:
                _double_blink_pending = false
                _double_blink_delay = 0.13
    elif _idle_enabled:
        if _double_blink_delay >= 0.0:
            _double_blink_delay -= delta
            if _double_blink_delay <= 0.0:
                _double_blink_delay = -1.0
                request_blink()
        else:
            _time_to_blink -= delta
            if _time_to_blink <= 0.0:
                _time_to_blink = _rng.randf_range(2.8, 6.5)
                _double_blink_pending = _rng.randf() < 0.18
                request_blink()

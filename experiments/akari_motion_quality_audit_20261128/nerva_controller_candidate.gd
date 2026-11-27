extends RefCounted
## NERVA movement controller extracted from the ACTUAL Dropbox v0.4 main.gd.
## v0.5: relaxed hands and coordinated autonomous idle; no Ghost intentions generated.
## Ghost is not required. No emotions or personal intention are generated here.
var skeleton: Skeleton3D
var mocap_player: AnimationPlayer
var mocap_enabled: bool = false
var mocap_failed: bool = false
var mocap_current: String = ""
var _mocap_completed_clip: String = ""
var _actual_walk_started: bool = false
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
# EXPERIMENTAL spatial motor control, not a new Ghost personality.
# Ghost picks semantic explore/sit/rise; NERVA owns joint timing and world movement.
var motion_root: Node3D
var _life_phase: float = 0.0
var _sit_amount: float = 0.0
var _walk_cycle: float = 0.0
var _walk_envelope: float = 0.0
var _gait_distance: float = 0.0
var _last_walk_x: float = 0.0
var _sitting_start: float = 0.0
var _seat_offset_z: float = -0.20 # Seat is behind the standing avatar on the trial stage.
var _life_target_x: float = 0.0
var _life_start_x: float = 0.0
var _life_finished: bool = false
var _life_serial: int = 0
var _hand_ratio: float = 0.90 # Reversible test; hand proportion feedback pending visual assessment.
var _base_pos: Dictionary = {}
var _base_scale: Dictionary = {}
var completed_life_actions: int = 0


# Ghost selects meaning; NERVA selects a body clip and manages its transition.
# Do not silently replace a missing Mocap clip with the previous artificial gait.
func bind_mocap(player: AnimationPlayer) -> bool:
    mocap_player = player
    if mocap_player != null and not mocap_player.animation_finished.is_connected(_on_mocap_animation_finished):
        mocap_player.animation_finished.connect(_on_mocap_animation_finished)
    if mocap_player == null:
        mocap_failed = true
        return false
    for mandatory in ["walk", "sit", "rise"]:
        if _find_clip(mandatory) == "":
            push_error("AKARI Mocap clip missing: " + mandatory)
            mocap_failed = true
            return false
    mocap_enabled = true
    mocap_failed = false
    _start_mocap("idle")
    # Preserve Kim’s approved reversible hand-size correction.
    for hand in ["leftHand", "rightHand"]:
        if _bones.has(hand):
            skeleton.set_bone_pose_scale(int(_bones[hand]), _base_scale[hand] * _hand_ratio)
    return true

func _find_clip(name: String) -> String:
    if mocap_player == null:
        return ""
    for clip in mocap_player.get_animation_list():
        var clean_name := str(clip).to_lower()
        if clean_name == name or clean_name.ends_with("/" + name):
            return str(clip)
    return ""

func _start_mocap(kind: String) -> void:
    if not mocap_enabled or mocap_player == null:
        return
    var selected := kind
    if kind == "explore": selected = "walk"
    elif kind == "attend": selected = "talk_idle" if _find_clip("talk_idle") != "" else "idle"
    elif kind == "scan": selected = "wait" if _find_clip("wait") != "" else "idle"
    elif kind == "rest": selected = "idle"
    elif kind == "drowsy": selected = "stretch" if _find_clip("stretch") != "" else "idle"
    elif kind == "sleep": selected = "idle"
    if kind == "idle" and _sit_amount >= 0.98:
        selected = "seated_idle"
        if _find_clip(selected) == "":
            # Hold last seated frame rather than instantly snapping to standing.
            mocap_player.pause()
            mocap_current = "seated_hold"
            return
    var clip := _find_clip(selected)
    if clip == "":
        if selected in ["walk", "sit", "rise"]:
            mocap_failed = true
            push_error("NERVA refuses to fake absent Mocap: " + selected)
        return
    var animation := mocap_player.get_animation(clip)
    if animation != null:
        if selected in ["walk", "idle", "seated_idle"]:
            animation.loop_mode = Animation.LOOP_LINEAR
        else:
            animation.loop_mode = Animation.LOOP_NONE
    _mocap_completed_clip = ""
    mocap_player.play(clip, 0.20)
    # NERVA owns speed and roots; never send joint angles from Ghost.
    if animation != null and selected in ["sit", "rise"]:
        mocap_player.speed_scale = maxf(0.1, animation.length / 3.0)
    elif animation != null and selected == "walk":
        mocap_player.speed_scale = 1.0
    else:
        mocap_player.speed_scale = 1.0
    mocap_current = selected

func accept_ghost_state(state: Dictionary) -> void:
    # Read only the six existing Heart compatibility values. NERVA never infers emotions.
    var clean: Dictionary = {}
    for key in ["mood", "affection", "curiosity", "boredom", "sleepiness", "attention"]:
        if state.has(key):
            var value: Variant = state[key]
            if (value is float or value is int) and is_finite(float(value)):
                clean[key] = clampf(float(value), 0.0, 1.0)
    _ghost_state = clean

func clear_ghost_state() -> void:
    _ghost_state.clear()

func accept_ghost_intent(kind: String, strength: float) -> void:
    if not kind in ["idle", "attend", "rest", "blink", "scan", "drowsy", "sleep", "explore", "sit", "rise"]:
        return
    if not is_finite(strength):
        return
    # Blink is a concurrent eyelid action, never a replacement for locomotion.
    if kind == "blink":
        if _ghost_intent != "sleep" and _blink_phase < 0.0:
            request_blink()
        return
    print("NERVA received Ghost intent: ", kind)
    if kind != _ghost_intent:
        _body_action_time = 0.0
        _gesture_timer = 0.0
    if kind in ["explore", "sit", "rise"]:
        _life_phase = 0.0
        _life_finished = false
        _life_serial += 1
        if motion_root != null:
            _life_start_x = motion_root.position.x
            _last_walk_x = _life_start_x
        if kind == "explore":
            _life_target_x = 0.0 if absf(_life_start_x + 0.65) <= 0.13 else -0.65
            _gait_distance = 0.0
            _walk_cycle = 0.0
            _actual_walk_started = false
        if kind == "sit":
            _sitting_start = _sit_amount
    if _ghost_intent == "sleep" and kind != "sleep":
        _blink_phase = 0.075
    _ghost_intent = kind
    # An exploring character turns on planted feet; do not walk-in-place
    # through the 0.8 s turn before physical translation begins.
    _start_mocap("idle" if kind == "explore" else kind)
    _ghost_strength = clampf(strength, 0.0, 1.0)
    _ghost_intent_remaining = -1.0 if kind in ["idle", "scan", "drowsy", "sleep", "explore", "sit", "rise"] else 3.0
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
    if _ghost_intent == "sleep":
        _blink_phase = 0.075 # Reopen from closed eyelids, not a sudden jump.
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
    _base_pos.clear()
    _base_scale.clear()
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
            _base_pos[n] = skeleton.get_bone_pose_position(idx)
            _base_scale[n] = skeleton.get_bone_pose_scale(idx)

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
    # When a calibrated clip drives the skeleton, do not overwrite its joints
    # with the legacy hand-coded gait/gesture pose. Face blink remains separate.
    if mocap_enabled:
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
    if _ghost_intent in ["explore", "sit", "rise"]:
        _body_action_name = _ghost_intent + " / spatial trial"
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
    _apply_life_pose(t)

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

func _ease(t: float) -> float:
    var v: float = clampf(t, 0.0, 1.0)
    return v * v * (3.0 - 2.0 * v)

func _advance_life(delta: float) -> void:
    if not _ghost_intent in ["explore", "sit", "rise"] or motion_root == null or _life_finished:
        return
    _life_phase += maxf(delta, 0.0)
    if _ghost_intent == "explore":
        # Progress and gait share the SAME travelled distance; a time-only
        # sinusoid on a translating root makes the feet skate on the floor.
        # AKARI's GLB has its face toward +Z (verified from Face/Hair geometry).
        # Motion destination is -X, so facing yaw is -PI/2, NOT +1.40.
        # Separate TURN -> MOVE -> TURN_BACK: root translation must not start
        # until the character is facing the travel direction.
        var travel := Vector3(_life_target_x - _life_start_x, 0.0, 0.0)
        var travel_yaw := atan2(travel.x, travel.z) # +Z model forward
        const TURN_SECONDS := 0.8
        const WALK_SECONDS := 4.4
        var walk_progress: float = 0.0
        if _life_phase < TURN_SECONDS:
            motion_root.rotation.y = lerp_angle(0.0, travel_yaw, _ease(_life_phase / TURN_SECONDS))
            motion_root.position.x = _life_start_x
            _walk_envelope = 0.0
        elif _life_phase < TURN_SECONDS + WALK_SECONDS:
            if mocap_enabled and mocap_current != "walk":
                _start_mocap("walk")
                _actual_walk_started = mocap_current == "walk" and mocap_player.is_playing()
            motion_root.rotation.y = travel_yaw
            walk_progress = _ease((_life_phase - TURN_SECONDS) / WALK_SECONDS)
            motion_root.position.x = lerpf(_life_start_x, _life_target_x, walk_progress)
            _walk_envelope = clampf(sin(PI * walk_progress) * 2.1, 0.0, 1.0)
        else:
            if mocap_enabled and mocap_current == "walk":
                _start_mocap("idle")
            motion_root.rotation.y = lerp_angle(travel_yaw, 0.0, _ease((_life_phase - TURN_SECONDS - WALK_SECONDS) / TURN_SECONDS))
            motion_root.position.x = _life_target_x
            _walk_envelope = 0.0
        var dx: float = motion_root.position.x - _last_walk_x
        _last_walk_x = motion_root.position.x
        _gait_distance += absf(dx)
        # The walk cycle advances only while travelling; no in-place backstep.
        _walk_cycle = _gait_distance * TAU / 0.27
        _body_action_name = "explore / face-correct turn-walk-turn trial"
        if _life_phase >= 6.0:
            _walk_envelope = 0.0
            motion_root.rotation.y = 0.0
            _life_finished = true
    elif _ghost_intent == "sit":
        # Seat phase is a coordinated squat/hinge, not instantaneous hips Y.
        _sit_amount = lerpf(_sitting_start, 1.0, _ease(_life_phase / 3.0))
        _body_action_name = "sitting / coordinated hinge trial"
        if _life_phase >= 3.0:
            _sit_amount = 1.0
            _life_finished = true
    elif _ghost_intent == "rise":
        _sit_amount = 1.0 - _ease(_life_phase / 3.0)
        _body_action_name = "rising / coordinated hinge trial"
        if _life_phase >= 3.0:
            _sit_amount = 0.0
            _life_finished = true
    if _life_finished:
        completed_life_actions += 1

func _apply_life_pose(_t: float) -> void:
    if skeleton == null:
        return
    var walking: float = _walk_envelope if _ghost_intent == "explore" and not _life_finished else 0.0
    var stride: float = sin(_walk_cycle)
    var left_swing: float = stride * walking
    var right_swing: float = -stride * walking
    var seat: float = clampf(_sit_amount, 0.0, 1.0)
    # Use the same continuous blend for legs, pelvis, torso and counter-swing.
    # This is NOT contact IK; it merely avoids independent contradictory cycles.
    if walking > 0.0 or seat > 0.0:
        _rot("leftUpperLeg", Quaternion(Vector3.RIGHT, deg_to_rad(22.0 * left_swing - 67.0 * seat)))
        _rot("rightUpperLeg", Quaternion(Vector3.RIGHT, deg_to_rad(22.0 * right_swing - 67.0 * seat)))
        # A knee flexes mainly during the swing half and releases at stance.
        _rot("leftLowerLeg", Quaternion(Vector3.RIGHT, deg_to_rad(26.0 * maxf(0.0, -left_swing) + 72.0 * seat)))
        _rot("rightLowerLeg", Quaternion(Vector3.RIGHT, deg_to_rad(26.0 * maxf(0.0, -right_swing) + 72.0 * seat)))
        _rot("leftFoot", Quaternion(Vector3.RIGHT, deg_to_rad(-12.0 * left_swing - 5.0 * seat)))
        _rot("rightFoot", Quaternion(Vector3.RIGHT, deg_to_rad(-12.0 * right_swing - 5.0 * seat)))
        # The pelvis and torso hinge together during seating; the torso
        # counterbalances the upper legs. Seat X is the movement destination.
        if _bones.has("hips"):
            skeleton.set_bone_pose_position(int(_bones["hips"]), _base_pos["hips"] + Vector3(0.0, -0.40 * seat + 0.009 * cos(_walk_cycle * 2.0) * walking, _seat_offset_z * seat))
        _rot("spine", Quaternion(Vector3.RIGHT, deg_to_rad(-12.0 * seat + 2.5 * walking)))
        _rot("chest", Quaternion(Vector3.RIGHT, deg_to_rad(9.0 * seat + _gesture_energy * (1.0 if _idle_enabled else 0.0) * sin(_breath_phase) * _breath_depth * 0.85)))
        # Arm motion opposes the swinging leg, while seated elbows settle.
        var left_arm: float = 72.0 + 14.0 * right_swing - 8.0 * seat
        var right_arm: float = -72.0 - 14.0 * left_swing + 8.0 * seat
        _rot("leftUpperArm", Quaternion(Vector3.FORWARD, deg_to_rad(left_arm)))
        _rot("rightUpperArm", Quaternion(Vector3.FORWARD, deg_to_rad(right_arm)))
        _rot("leftLowerArm", Quaternion(Vector3.UP, deg_to_rad(-8.0 - 15.0 * seat)))
        _rot("rightLowerArm", Quaternion(Vector3.UP, deg_to_rad(8.0 + 15.0 * seat)))
    else:
        # Avoid leaving the model in the previous action's hip offset.
        if _bones.has("hips"):
            skeleton.set_bone_pose_position(int(_bones["hips"]), _base_pos["hips"])
    # Reversible test. GLB, textures, clothes and original rest-pose unchanged.
    for hand_name in ["leftHand", "rightHand"]:
        if _bones.has(hand_name):
            skeleton.set_bone_pose_scale(int(_bones[hand_name]), _base_scale[hand_name] * _hand_ratio)

# A timer is never evidence that a clip really played to its last frame.
# The engine's animation_finished signal is the source of truth for sit/rise.
func _on_mocap_animation_finished(clip: StringName) -> void:
    if str(clip) == _find_clip(mocap_current):
        _mocap_completed_clip = str(clip)

func _mocap_terminal_ready(kind: String) -> bool:
    if mocap_player == null or not mocap_enabled or mocap_failed:
        return false
    if kind == "explore":
        # Walk is looped, so its end is the physical destination.
        # Walk must have genuinely played during translation. The final
        # turn-back runs a planted-foot idle and must not fail completion.
        return _actual_walk_started and mocap_current == "idle"
    var expected := "sit" if kind == "sit" else "rise"
    if mocap_current != expected:
        return false
    var clip := _find_clip(expected)
    if clip == "":
        return false
    return _mocap_completed_clip == clip

func motion_terminated(kind: String, serial: int) -> bool:
    return kind == _ghost_intent and serial == _life_serial and _life_finished

func motion_finished(kind: String, serial: int) -> bool:
    if not mocap_enabled or mocap_failed:
        return false
    # Not a quality/ground-contact check. Confirm the motor actually reached
    # the measured trial-stage target before telling Ghost it succeeded.
    if not motion_terminated(kind, serial) or motion_root == null:
        return false
    if not _mocap_terminal_ready(kind):
        return false
    if kind == "explore":
        return absf(motion_root.position.x - _life_target_x) <= 0.025 and absf(motion_root.rotation.y) < 0.03
    if kind == "sit":
        return absf(motion_root.position.x + 0.65) <= 0.13 and _sit_amount >= 0.99
    if kind == "rise":
        return absf(motion_root.position.x + 0.65) <= 0.13 and _sit_amount <= 0.01
    return false

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
    if attention_points.is_empty():
        _gaze_countdown = 3.0
        return
    var selected: int
    if _last_focus >= 0 and _last_focus < attention_points.size() and attention_points.size() > 1:
        selected = _rng.randi_range(0, attention_points.size() - 2)
        if selected >= _last_focus:
            selected += 1
    else:
        selected = _rng.randi_range(0, attention_points.size() - 1)
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
    if not is_finite(delta) or delta <= 0.0:
        return
    # Bounded substeps preserve brief eyelid phases and smoothing after stalls.
    var remaining := minf(delta, 0.25)
    while remaining > 0.000001:
        var dt := minf(remaining, 1.0 / 120.0)
        _step_motor(dt)
        remaining -= dt

func _step_motor(delta: float) -> void:
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
    _advance_life(delta)
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

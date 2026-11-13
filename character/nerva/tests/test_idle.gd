extends SceneTree
const Motor = preload("res://nerva_idle.gd")
var checks: int = 0
var failures: int = 0

func check(value: bool, description: String) -> void:
	checks += 1
	if not value:
		failures += 1
		push_error(description)

func _initialize() -> void:
	call_deferred("run_tests")

func make_rig(seed_value: int) -> Dictionary:
	var root_node := Node3D.new()
	root_node.name = "Fixture"
	root.add_child(root_node)
	var skeleton := Skeleton3D.new()
	skeleton.name = "Skeleton3D"
	root_node.add_child(skeleton)
	for bone in ["Chest", "Neck", "Head", "LeftShoulder", "RightShoulder", "LeftEye", "RightEye"]:
		skeleton.add_bone(bone)
	var motor := Node3D.new()
	motor.name = "Nerva"
	motor.set_script(Motor)
	motor.skeleton_path = NodePath("../Skeleton3D")
	motor.random_seed = seed_value
	root_node.add_child(motor)
	motor.set_process(false)
	return {"root": root_node, "skeleton": skeleton, "motor": motor}

func run_tests() -> void:
	var rig := make_rig(1042)
	var skeleton: Skeleton3D = rig.skeleton
	var motor: Node3D = rig.motor
	check(motor.diagnostics().bound, "Body binding must succeed")
	check(motor.diagnostics().bone_count == 7, "Expected seven mapped bones")
	check(motor.diagnostics().elapsed == 0.0, "New motor starts with zero elapsed time")
	motor.step_idle(0.0)
	check(motor.diagnostics().elapsed == 0.0, "Zero delta must not move the motor")
	var changed_pose := false
	var blink_seen := false
	var pose_samples: Array = []
	for i in range(4800):
		motor.step_idle(1.0 / 60.0)
		var info: Dictionary = motor.diagnostics()
		blink_seen = blink_seen or info.blink > 0.25
		var head_rotation := skeleton.get_bone_pose_rotation(skeleton.find_bone("Head"))
		if head_rotation.angle_to(Quaternion.IDENTITY) > 0.002:
			changed_pose = true
		if i % 600 == 0:
			pose_samples.append(head_rotation)
		check(absf(info.gaze.x) <= 0.73 and absf(info.gaze.y) <= 0.35, "Eye movement stays limited")
		check(info.blink >= 0.0 and info.blink <= 1.0, "Blink value remains in blend shape range")
	check(changed_pose, "Motor must generate continuous nonzero pose motion")
	check(blink_seen, "Irregular autonomous blink should occur during 80 seconds")
	check(motor.diagnostics().elapsed > 79.9, "Motor clocks real elapsed update time")
	var distinct := false
	for i in range(1, pose_samples.size()):
		if pose_samples[i].angle_to(pose_samples[i - 1]) > 0.006:
			distinct = true
	check(distinct, "Head motion does not freeze at one pose")
	check(skeleton.get_bone_pose_rotation(skeleton.find_bone("Head")).angle_to(Quaternion.IDENTITY) < 0.4, "Head movement stays within safe limits")
	check(skeleton.get_bone_pose_rotation(skeleton.find_bone("LeftShoulder")).angle_to(Quaternion.IDENTITY) < 0.25, "Shoulder motion is subtle")
	# No Ghost or WORLD nodes are required for this fixture.
	check(root.get_node_or_null("Ghost") == null, "Standalone idle never depends on Ghost")
	check(root.get_node_or_null("WORLD") == null, "Standalone idle never depends on WORLD")
	rig.root.queue_free()
	await process_frame
	print("NERVA idle: %d checks, %d failures" % [checks, failures])
	quit(0 if failures == 0 else 1)

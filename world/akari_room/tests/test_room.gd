extends SceneTree
const Clock = preload("res://scripts/world_clock.gd")
const State = preload("res://scripts/world_state.gd")
var failures := 0
var checks := 0
var now := 0

func check(condition: bool, message: String) -> void:
	checks += 1
	if not condition:
		failures += 1
		push_error(message)

func _initialize() -> void:
	call_deferred("run")

func run() -> void:
	var clock = Clock.new(func(): return now)
	# UTC 2026-11-12 15:00 -> JST next day, regardless of OS timezone.
	now = int(Time.get_unix_time_from_datetime_string("2026-11-12T15:00:00"))
	var jst: Dictionary = clock.display_jst()
	check(jst.day == 13 and jst.hour == 0 and jst.weekday == 5, "JST midnight/weekday")
	now = int(Time.get_unix_time_from_datetime_string("2028-02-28T15:00:00"))
	check(clock.display_jst().month == 2 and clock.display_jst().day == 29, "JST leap day")
	now = int(Time.get_unix_time_from_datetime_string("2026-12-31T15:00:00"))
	check(clock.display_jst().year == 2027, "JST year rollover")
	var state = State.new()
	var before: Dictionary = state.snapshot_for_admin()
	var copy: Dictionary = state.snapshot_for_admin()
	copy.objects.bed.position[0] = 900
	copy.events.append("invented_future")
	check(state.snapshot_for_admin() == before, "Nested state copies cannot mutate canonical state")
	clock.preview_at(now + 86400 * 400)
	check(clock.is_preview() and clock.display_utc() == now + 86400 * 400, "Future presentation time")
	check(clock.current_utc() == now, "Preview must not move real clock")
	now += 30
	check(clock.current_utc() == now, "Real clock continues during preview")
	check(state.snapshot_for_admin() == before, "Preview cannot change placements/history")
	clock.preview_at(now - 86400)
	check(clock.display_utc() < clock.current_utc(), "Past preview")
	clock.return_to_present()
	check(not clock.is_preview() and clock.display_utc() == now, "Return to current time, not entry timestamp")
	var room = load("res://scenes/akari_room.tscn").instantiate()
	root.add_child(room)
	await process_frame
	check(room.camera.current, "Active room camera")
	check(room.environment.environment != null, "Environment resource")
	for id in ["desk", "bookshelf", "bed", "low_table"]:
		check(room.has_node("Furniture/" + id), "Required 3D furniture: " + id)
		var p: Array = room.world_state.snapshot_for_admin().objects[id].position
		check(room.get_node("Furniture/" + id).position.is_equal_approx(Vector3(p[0],p[1],p[2])), "Canonical seed placement: " + id)
	check(room.has_node("Furniture/desk/LaptopScreen"), "Single PC screen")
	check(room.has_node("Furniture/bookshelf/Book0_0"), "Actual 3D books")
	check(room.has_node("Architecture/Window"), "Open window frame")
	check(room.has_node("TokyoInspiredSkyline/TowerAntenna"), "Tokyo-inspired skyline")
	check(room.find_children("*", "CollisionShape3D", true, false).size() >= 12, "Static room/furniture collisions")
	check(room.find_children("*Cat*", "", true, false).is_empty(), "No lucky cat")
	await physics_frame
	await physics_frame
	var ray := PhysicsRayQueryParameters3D.create(Vector3(0.0, 2.0, 2.5), Vector3(0.0, -1.0, 2.5))
	check(not room.get_world_3d().direct_space_state.intersect_ray(ray).is_empty(), "Floor responds to physics query")
	for id in ["desk", "bookshelf", "bed", "low_table"]:
		var center: Vector3 = room.get_node("Furniture/" + id).global_position + Vector3(0, 0.8, 0)
		check(not room.camera.is_position_behind(center) and root.get_visible_rect().has_point(room.camera.unproject_position(center)), "Furniture inside initial camera framing: " + id)
	var original: Dictionary = room.world_state.snapshot_for_admin()
	room.clock.preview_at(int(Time.get_unix_time_from_datetime_string("2026-11-12T03:00:00")))
	await process_frame
	await process_frame
	var noon_energy: float = room.sun.light_energy
	room.clock.preview_at(int(Time.get_unix_time_from_datetime_string("2026-11-12T15:00:00")))
	await process_frame
	await process_frame
	check(room.sun.light_energy < noon_energy, "Night preview updates environmental light")
	check(room.world_state.snapshot_for_admin() == original, "Rendered preview preserves world state")
	check(room._mode.text.contains("未接続"), "Weather unavailable is visible")
	var event := InputEventKey.new()
	event.keycode = KEY_ESCAPE
	event.pressed = true
	room._unhandled_input(event)
	check(not room.clock.is_preview(), "Escape returns to present")
	var original_camera: Vector3 = room.camera.position
	var wheel := InputEventMouseButton.new()
	wheel.button_index = MOUSE_BUTTON_WHEEL_UP
	wheel.pressed = true
	room._unhandled_input(wheel)
	check(not room.camera.position.is_equal_approx(original_camera), "Camera zoom changes viewpoint")
	event.keycode = KEY_R
	room._unhandled_input(event)
	check(room.camera.position.is_equal_approx(original_camera), "Camera reset")
	room.queue_free()
	await process_frame
	print("AKARI Room: %d checks, %d failures" % [checks, failures])
	quit(0 if failures == 0 else 1)

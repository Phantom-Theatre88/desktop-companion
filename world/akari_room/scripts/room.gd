extends Node3D
const Clock = preload("res://scripts/world_clock.gd")
const State = preload("res://scripts/world_state.gd")
const ArtPass = preload("res://scripts/art_pass.gd")
const BedAssetLoader = preload("res://scripts/bed_asset_loader.gd")
var clock = Clock.new()
var world_state = State.new()
var _yaw := 0.0
var _pitch := 0.0
var _distance := 6.3
var _dragging := false
var _label: Label
var _mode: Label
var _sky: ProceduralSkyMaterial
var _capture_path := ""
var _capture_frames := 0
@onready var camera: Camera3D = $Camera3D
@onready var environment: WorldEnvironment = $WorldEnvironment
@onready var sun: DirectionalLight3D = $Sun

func _ready() -> void:
	# The canonical layout drives the initial rendered placements.
	for id in world_state.snapshot_for_admin().objects:
		var p: Array = world_state.snapshot_for_admin().objects[id].position
		get_node("Furniture/" + id).position = Vector3(p[0], p[1], p[2])
	# Art pass changes display meshes only; canonical world state is untouched.
	ArtPass.apply(self)
	# Prefer Blender-authored asset if present; fallback leaves existing room intact.
	BedAssetLoader.apply(self)
	_sky = environment.environment.sky.sky_material
	_make_ui()
	_update_camera()
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("--capture="):
			_capture_path = arg.trim_prefix("--capture=")
		if arg.begins_with("--preview-utc="):
			clock.preview_at(int(arg.trim_prefix("--preview-utc=")))
	if not _capture_path.is_empty() and DisplayServer.get_name() == "headless":
		push_error("Screenshot requires a graphical display; headless uses dummy rendering.")
		get_tree().quit(1)
	_update_environment()

func _make_ui() -> void:
	var layer := CanvasLayer.new()
	add_child(layer)
	var panel := PanelContainer.new()
	panel.position = Vector2(18, 14)
	panel.scale = Vector2(0.80, 0.80)
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.10, 0.13, 0.15, 0.64)
	style.content_margin_left = 18
	style.content_margin_right = 18
	style.content_margin_top = 12
	style.content_margin_bottom = 12
	style.set_corner_radius_all(10)
	panel.add_theme_stylebox_override("panel", style)
	layer.add_child(panel)
	var column := VBoxContainer.new()
	panel.add_child(column)
	var title := Label.new()
	title.text = "AKARI Room"
	title.add_theme_font_size_override("font_size", 20)
	column.add_child(title)
	_label = Label.new()
	column.add_child(_label)
	_mode = Label.new()
	column.add_child(_mode)
	var buttons := HBoxContainer.new()
	column.add_child(buttons)
	for spec in [["+6h", 21600], ["+24h", 86400]]:
		var button := Button.new()
		button.text = spec[0]
		var offset: int = spec[1]
		button.pressed.connect(func(): clock.preview_at(clock.current_utc() + offset))
		buttons.add_child(button)
	var present := Button.new()
	present.text = "現在へ戻る"
	present.pressed.connect(clock.return_to_present)
	buttons.add_child(present)
	var hint := Label.new()
	hint.text = "右ドラッグ: 見回す  /  ホイール: 拡大  /  R: 視点リセット  /  Esc: 現在"
	hint.position = Vector2(22, 766)
	hint.add_theme_font_size_override("font_size", 13)
	layer.add_child(hint)

func _process(_delta: float) -> void:
	_update_environment()
	if not _capture_path.is_empty():
		_capture_frames += 1
		if _capture_frames == 12:
			await RenderingServer.frame_post_draw
			var error := get_viewport().get_texture().get_image().save_png(_capture_path)
			print("CAPTURE ", _capture_path, " result=", error)
			get_tree().quit(0 if error == OK else 1)

func _update_environment() -> void:
	var t: Dictionary = clock.display_jst()
	var hour := float(t.hour) + float(t.minute) / 60.0
	# Artistic day/night approximation, NOT Tokyo solar ephemeris or weather.
	var daylight := clampf(sin((hour - 6.0) / 12.0 * PI), 0.0, 1.0)
	var dusk := clampf(1.0 - absf(hour - 17.0) / 2.5, 0.0, 1.0)
	_sky.sky_top_color = Color("101c38").lerp(Color("719cc2"), daylight)
	_sky.sky_horizon_color = Color("283a5a").lerp(Color("e6b19c"), maxf(daylight * 0.65, dusk))
	_sky.ground_horizon_color = _sky.sky_horizon_color
	environment.environment.ambient_light_energy = 0.32 + daylight * 0.42
	sun.light_energy = 0.12 + daylight * 0.85
	sun.light_color = Color("ffd0a0").lerp(Color("fff0df"), daylight)
	_label.text = clock.display_label()
	_mode.text = ("時刻プレビュー（生活状態は変更しません）" if clock.is_preview() else "現在のWORLD") + "\n東京都の天気: 未接続 / 光は時刻による概略表現"

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_RIGHT:
			_dragging = event.pressed
		if event.pressed and event.button_index == MOUSE_BUTTON_WHEEL_UP:
			_distance = clampf(_distance - 0.4, 5.0, 9.0)
		if event.pressed and event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			_distance = clampf(_distance + 0.4, 5.0, 9.0)
	if event is InputEventMouseMotion and _dragging:
		_yaw = clampf(_yaw - event.relative.x * 0.003, -0.28, 0.28)
		_pitch = clampf(_pitch + event.relative.y * 0.003, -0.08, 0.30)
	if event is InputEventKey and event.pressed:
		if event.keycode == KEY_ESCAPE:
			clock.return_to_present()
		if event.keycode == KEY_R:
			_yaw = 0.0
			_pitch = 0.0
			_distance = 6.3
	_update_camera()

func _update_camera() -> void:
	var target := Vector3(0.0, 1.35, -0.6)
	camera.position = target + Vector3(sin(_yaw) * _distance, 1.0 + _pitch * _distance, cos(_yaw) * _distance)
	camera.look_at(target)

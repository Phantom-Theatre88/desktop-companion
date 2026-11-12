extends RefCounted
## UTC epoch internally; explicit JST conversion, independent of host timezone.
## Preview changes presentation only. Never feeds Ghost Time or simulation dt.
const JST_OFFSET_SECONDS := 9 * 60 * 60
var _utc_now: Callable
var _preview_utc: int = 0
var _preview_active := false

func _init(utc_now: Callable = Callable()) -> void:
	_utc_now = utc_now if utc_now.is_valid() else func(): return Time.get_unix_time_from_system()

func current_utc() -> int:
	return int(_utc_now.call())

func display_utc() -> int:
	return _preview_utc if _preview_active else current_utc()

func preview_at(utc_seconds: int) -> void:
	_preview_utc = utc_seconds
	_preview_active = true

func return_to_present() -> void:
	_preview_active = false

func is_preview() -> bool:
	return _preview_active

func jst_at(utc_seconds: int) -> Dictionary:
	return Time.get_datetime_dict_from_unix_time(utc_seconds + JST_OFFSET_SECONDS)

func display_jst() -> Dictionary:
	return jst_at(display_utc())

func display_label() -> String:
	var t := display_jst()
	return "%04d/%02d/%02d  %02d:%02d:%02d JST" % [t.year, t.month, t.day, t.hour, t.minute, t.second]

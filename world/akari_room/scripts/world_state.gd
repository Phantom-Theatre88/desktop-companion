extends RefCounted
## WORLD-owned canonical seed, never attached to a preview instance or Ghost.
## Snapshot is diagnostic/admin data, NOT a perception/transport API.
## Disk persistence and authorized action adjudication are future work.
var _canonical := {
	"schema_version": 1,
	"room_id": "akari_room",
	"objects": {
		"desk": {"position": [-1.8, 0.0, -1.15]},
		"bookshelf": {"position": [-2.78, 0.0, 0.25]},
		"bed": {"position": [2.05, 0.0, -0.15]},
		"low_table": {"position": [0.0, 0.0, 0.9]}
	},
	"environment": {"weather_status": "unavailable", "region": "Tokyo"},
	"events": []
}

func snapshot_for_admin() -> Dictionary:
	return _canonical.duplicate(true)

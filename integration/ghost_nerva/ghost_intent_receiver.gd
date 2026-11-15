extends Node
## Add as a child of the Character v0.4 main Node3D in a COPY of the Godot scene.
## Ghost supplies meaning only; NERVA alone generates pose, timing, and gaze.

@export var url: String = "ws://127.0.0.1:8766"
var _socket := WebSocketPeer.new()
var _retry_time := 0.0
var _was_connected := false
var _last_sequence := -1

func _ready() -> void:
    _socket.connect_to_url(url)

func _process(delta: float) -> void:
    _socket.poll()
    match _socket.get_ready_state():
        WebSocketPeer.STATE_OPEN:
            _was_connected = true
            _retry_time = 0.0
            while _socket.get_available_packet_count() > 0:
                var message = JSON.parse_string(_socket.get_packet().get_string_from_utf8())
                if message is Dictionary:
                    _handle(message)
        WebSocketPeer.STATE_CLOSED:
            if _was_connected:
                _was_connected = false
                get_parent().get("_nerva").clear_ghost_intent()
                get_parent().get("_nerva").clear_ghost_state()
            _retry_time += delta
            if _retry_time >= 3.0:
                _retry_time = 0.0
                _last_sequence = -1
                _socket = WebSocketPeer.new()
                _socket.connect_to_url(url)

func _handle(message: Dictionary) -> void:
    if message.get("type") == "ghost_snapshot" and int(message.get("version", -1)) == 1:
        if message.get("source") != "ghost_autonomy_compatibility_spike":
            return
        var state = message.get("autonomy")
        if not state is Dictionary:
            return
        var boredom = state.get("boredom")
        var sleepiness = state.get("sleepiness")
        if not (boredom is float or boredom is int) or not (sleepiness is float or sleepiness is int):
            return
        get_parent().get("_nerva").accept_ghost_state(float(boredom), float(sleepiness))
        return
    if message.get("type") != "ghost_intent" or int(message.get("version", -1)) != 1:
        return
    var seq := int(message.get("sequence", -1))
    if seq < _last_sequence:
        return
    var intent = message.get("intent")
    if not intent is Dictionary:
        return
    var kind := str(intent.get("kind", ""))
    if not kind in ["idle", "attend", "rest", "blink", "scan", "drowsy", "sleep"]:
        return
    _last_sequence = seq
    get_parent().get("_nerva").accept_ghost_intent(kind, clampf(float(intent.get("strength", 0.0)), 0.0, 1.0))

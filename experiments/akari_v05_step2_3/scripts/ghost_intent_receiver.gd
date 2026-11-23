extends Node
## v0.5 integration; send feedback only after a NERVA-driven frame on a bound model.
## Ghost supplies meaning only; NERVA alone generates pose, timing, and gaze.

@export var url: String = "ws://127.0.0.1:8766"
var _socket := WebSocketPeer.new()
var _retry_time := 0.0
var _was_connected := false
var _last_sequence := -1
var _pending_result: int = -1
var _pending_kind: String = ""
var _pending_frame: int = -1

func _ready() -> void:
    print("Ghost receiver connecting: ", url)
    print("Ghost receiver connect result: ", _socket.connect_to_url(url))

func _process(delta: float) -> void:
    _socket.poll()
    match _socket.get_ready_state():
        WebSocketPeer.STATE_OPEN:
            if not _was_connected:
                print("Ghost receiver connected")
            _was_connected = true
            _retry_time = 0.0
            while _socket.get_available_packet_count() > 0:
                var message = JSON.parse_string(_socket.get_packet().get_string_from_utf8())
                if message is Dictionary:
                    _handle(message)
            _send_applied_frame_if_ready()
        WebSocketPeer.STATE_CLOSED:
            if _was_connected:
                print("Ghost receiver disconnected")
            if _was_connected:
                _was_connected = false
                get_parent().get("_nerva").clear_ghost_intent()
                _pending_result = -1
            _retry_time += delta
            if _retry_time >= 3.0:
                _retry_time = 0.0
                _last_sequence = -1
                _socket = WebSocketPeer.new()
                _socket.connect_to_url(url)

func _handle(message: Dictionary) -> void:
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
    # Return result for a displaced transient; no false completion of persistent intents.
    if _pending_result >= 0 and _pending_result != seq:
        _send_result(_pending_result, "interrupted", false)
    _last_sequence = seq
    var motor = get_parent().get("_nerva")
    motor.accept_ghost_intent(kind, clampf(float(intent.get("strength", 0.0)), 0.0, 1.0))
    _pending_result = seq
    _pending_kind = kind
    _pending_frame = Engine.get_process_frames()

func _send_applied_frame_if_ready() -> void:
    if _pending_result < 0 or Engine.get_process_frames() <= _pending_frame:
        return
    var motor = get_parent().get("_nerva")
    # This is a verified bound frame only, NOT confirmation of visual quality.
    if motor.skeleton != null and motor._bones.size() > 0:
        _send_result(_pending_result, "applied_frame", true)
    else:
        _send_result(_pending_result, "rejected", false)
    _pending_result = -1

func _send_result(seq: int, status: String, bound: bool) -> void:
    if _socket.get_ready_state() != WebSocketPeer.STATE_OPEN:
        return
    var response := {"type": "nerva_result", "version": 1,
                    "sequence": seq, "status": status, "model_bound": bound}
    _socket.send_text(JSON.stringify(response))
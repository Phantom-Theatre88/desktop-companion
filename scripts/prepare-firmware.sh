#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
target_dir="${1:-"$repo_root/build/stackchan"}"

if [[ -e "$target_dir" ]]; then
    printf 'Refusing to overwrite existing build directory: %s\n' "$target_dir" >&2
    exit 1
fi

mkdir -p "$(dirname "$target_dir")"
cp -R "$repo_root/upstream/stackchan" "$target_dir"

relative_target="${target_dir#$repo_root/}"
if [[ "$relative_target" == "$target_dir" ]]; then
    printf 'Build directory must be inside the repository: %s\n' "$target_dir" >&2
    exit 1
fi

git -C "$repo_root" apply --directory="$relative_target" "$repo_root/patches/yuki-stackchan-integration.patch"
rsync -a "$repo_root/firmware/yuki/" "$target_dir/firmware/"

# Step 2 local vision: avoid fragile patch hunks and apply the required
# transformations deterministically to the prepared workspace.
display_file="$target_dir/firmware/main/hal/board/stackchan_display.cc"
vision_file="$target_dir/firmware/main/stackchan/vision/yuki_vision.cpp"

python3 - "$display_file" "$vision_file" <<'PY'
from pathlib import Path
import sys

display = Path(sys.argv[1])
vision = Path(sys.argv[2])

# 1) Start and enable vision immediately at boot, independent of Xiaozhi
# conversation state. There is already a second EnableYukiVision() in the
# LISTENING status path, so we must check the specific SetupUI sequence rather
# than merely checking whether the function name appears anywhere in the file.
display_text = display.read_text()
old = "    StartYukiVision();\n    StartYukiCuriosity();"
new = "    StartYukiVision();\n    EnableYukiVision();\n    StartYukiCuriosity();"
if new not in display_text:
    if old not in display_text:
        raise SystemExit("Unable to locate StartYukiVision() insertion point")
    display_text = display_text.replace(old, new, 1)
    display.write_text(display_text)

# 2) Physical face tracking is a body-local reflex. Xiaozhi state may pause
# motion during user speech, but must never gate head tracking itself.
vision_text = vision.read_text()
old = '''    // Before wake-up Yuki may look with her eyes, but the physical head stays\n    // still. Once the conversation is active, face tracking owns head motion.\n    const bool conversation_active = hal_bridge::is_xiaozhi_ready() && !hal_bridge::is_xiaozhi_idle();\n    if (!conversation_active) {\n        voice_pause_until_ = 0;\n        voice_pause_latched_ = false;\n        stackchan.motion().setModifyLock(false);\n        if (head_tracking_) {\n            stackchan.motion().setAutoAngleSyncEnabled(true);\n            head_tracking_ = false;\n        }\n        return;\n    }\n\n    const bool voice_detected = hal_bridge::is_xiaozhi_voice_detected();\n    if (voice_detected && !voice_pause_latched_) {\n        ESP_LOGI(kTag, "User speech detected; briefly pausing physical head");\n        voice_pause_until_ = now + 1200;\n        voice_pause_latched_ = true;\n    } else if (!voice_detected) {\n        voice_pause_latched_ = false;\n    }\n    if (static_cast<int32_t>(voice_pause_until_ - now) > 0) {\n        // The pause is edge-triggered and bounded. A stale VAD signal must not\n        // keep the physical head frozen for the rest of the conversation.\n        stackchan.motion().setModifyLock(false);\n        return;\n    }\n    stackchan.motion().setModifyLock(true);\n'''
new = '''    // Kim edition: face following is a body-local reflex. It must work even\n    // when Wi-Fi/Xiaozhi is unavailable. Conversation state only affects the\n    // optional voice-pause behaviour; it never gates physical face tracking.\n    const bool conversation_active = hal_bridge::is_xiaozhi_ready() && !hal_bridge::is_xiaozhi_idle();\n    if (conversation_active) {\n        const bool voice_detected = hal_bridge::is_xiaozhi_voice_detected();\n        if (voice_detected && !voice_pause_latched_) {\n            ESP_LOGI(kTag, "User speech detected; briefly pausing physical head");\n            voice_pause_until_ = now + 1200;\n            voice_pause_latched_ = true;\n        } else if (!voice_detected) {\n            voice_pause_latched_ = false;\n        }\n        if (static_cast<int32_t>(voice_pause_until_ - now) > 0) {\n            stackchan.motion().setModifyLock(false);\n            return;\n        }\n    } else {\n        voice_pause_until_ = 0;\n        voice_pause_latched_ = false;\n    }\n    stackchan.motion().setModifyLock(true);\n'''
if old in vision_text:
    vision_text = vision_text.replace(old, new, 1)
elif "Kim edition: face following is a body-local reflex" not in vision_text:
    raise SystemExit("Unable to locate Xiaozhi face-tracking gate; refusing to prepare firmware")

# 3) Temporary coordinate-mapping test.
# The real-device log shows raw face X stuck near 26 while raw Y varies widely,
# so the camera/detector coordinates appear rotated relative to the robot's
# physical left/right axis. For this test build only:
#   mapped X = raw Y scaled to the 320-wide logical frame
#   mapped Y = frame center (pitch frozen)
# This lets us verify left/right tracking safely without driving pitch to an
# extreme. The log prints both raw and mapped coordinates.
old = '''                    if (stable_face_samples >= 3) {\n                        face_x.store(static_cast<int>(filtered_face_x));\n                        face_y.store(static_cast<int>(filtered_face_y));\n                        frame_width.store(width);\n                        frame_height.store(height);\n                        face_seen_at.store(now);\n                    }\n'''
new = '''                    if (stable_face_samples >= 3) {\n                        const int raw_x = static_cast<int>(filtered_face_x);\n                        const int raw_y = static_cast<int>(filtered_face_y);\n                        const int mapped_x = std::clamp(\n                            static_cast<int>(filtered_face_y * static_cast<float>(width) / static_cast<float>(height)),\n                            0, width - 1);\n                        const int mapped_y = height / 2;\n                        face_x.store(mapped_x);\n                        face_y.store(mapped_y);\n                        frame_width.store(width);\n                        frame_height.store(height);\n                        face_seen_at.store(now);\n                        if (stable_face_samples == 3) {\n                            ESP_LOGI(kTag,\n                                     "Coordinate test raw=(%d,%d) mapped=(%d,%d) frame=%dx%d",\n                                     raw_x, raw_y, mapped_x, mapped_y, width, height);\n                        }\n                    }\n'''
if new not in vision_text:
    if old not in vision_text:
        raise SystemExit("Unable to locate face-coordinate storage block")
    vision_text = vision_text.replace(old, new, 1)

old = '''        ESP_LOGI(kTag, "Stable face detected at (%d,%d) in %dx%d frame", face_x.load(), face_y.load(), width, height);\n'''
new = '''        ESP_LOGI(kTag, "Stable mapped face at (%d,%d) in %dx%d frame", face_x.load(), face_y.load(), width, height);\n'''
if new not in vision_text:
    if old not in vision_text:
        raise SystemExit("Unable to locate stable-face log line")
    vision_text = vision_text.replace(old, new, 1)

vision.write_text(vision_text)
PY

# Verify local vision invariants before continuing.
python3 - "$display_file" "$vision_file" <<'PY'
from pathlib import Path
import sys

display_text = Path(sys.argv[1]).read_text()
vision_text = Path(sys.argv[2]).read_text()
required = "    StartYukiVision();\n    EnableYukiVision();\n    StartYukiCuriosity();"
if required not in display_text:
    raise SystemExit("Boot-time EnableYukiVision() was not inserted next to StartYukiVision()")
if "Coordinate test raw=" not in vision_text:
    raise SystemExit("Coordinate-mapping test instrumentation was not inserted")
PY
grep -q 'Kim edition: face following is a body-local reflex' "$vision_file"
if grep -q 'if (!conversation_active)' "$vision_file"; then
    printf 'Offline face-tracking gate is still present in %s\n' "$vision_file" >&2
    exit 1
fi

# Step 2 compatibility: Yuki integration was written against a Xiaozhi API
# that is not present in the pinned v2.2.4 source. Apply this deterministically
# to the prepared workspace and verify it before build.
compat_file="$target_dir/firmware/main/hal/board/stackchan.cc"
python3 - "$compat_file" <<'PY'
from pathlib import Path
import sys

path = Path(sys.argv[1])
text = path.read_text()
old = "    Application::GetInstance().StartProactiveConversation(prompt);"
new = "    // Step 2 compatibility shim: proactive conversation is deferred to a later step.\n    (void)prompt;"
if old in text:
    text = text.replace(old, new, 1)
    path.write_text(text)
elif "StartProactiveConversation(prompt)" in text:
    raise SystemExit("Unexpected proactive-conversation call shape; refusing to prepare firmware")
PY

if grep -q 'StartProactiveConversation(prompt)' "$compat_file"; then
    printf 'Compatibility replacement failed in %s\n' "$compat_file" >&2
    exit 1
fi

mkdir -p "$target_dir/firmware/patches"
cp "$repo_root/patches/xiaozhi-esp32.patch" "$target_dir/firmware/patches/"

printf 'Prepared firmware workspace at %s\n' "$target_dir"

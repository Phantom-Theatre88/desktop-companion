#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
vision_file="$repo_root/build/stackchan/firmware/main/stackchan/vision/yuki_vision.cpp"

if [[ ! -f "$vision_file" ]]; then
  echo "Generated firmware source not found: $vision_file" >&2
  echo "Run ./scripts/prepare-firmware.sh first." >&2
  exit 1
fi

python3 - "$vision_file" <<'PY'
from pathlib import Path
import sys

path = Path(sys.argv[1])
text = path.read_text()

# 1) Accept the detector's actual face container type.
old = "void emit_debug_frame(const uint8_t* frame, int width, int height, int format, const std::vector<dl::detect::result_t>& faces,\n"
new = "template <typename FaceContainer>\nvoid emit_debug_frame(const uint8_t* frame, int width, int height, int format, const FaceContainer& faces,\n"
if old in text:
    text = text.replace(old, new, 1)
elif new not in text:
    raise SystemExit("emit_debug_frame signature not found; refusing to guess")

# 2) The Mac debug image must represent the exact bytes immediately BEFORE
# HumanFaceDetect::run(image).  Remove the old sampling done after run().
old_sampling = '''{\n    for (int y = 0; y < kDebugHeight; ++y) {\n        const int source_y = y * height / kDebugHeight;\n        for (int x = 0; x < kDebugWidth; ++x) {\n            const int source_x = x * width / kDebugWidth;\n            debug_gray[y * kDebugWidth + x] = sample_luma(frame, width, source_x, source_y, format);\n        }\n    }\n\n    int x1 = -1;\n'''
new_sampling = '''{\n    // debug_gray was captured before detector->run(image).\n    (void)frame;\n    (void)format;\n\n    int x1 = -1;\n'''
if old_sampling in text:
    text = text.replace(old_sampling, new_sampling, 1)
elif "debug_gray was captured before detector->run(image)" not in text:
    raise SystemExit("emit_debug_frame sampling block not found; refusing to guess")

# 3) Capture the debug raster before the detector gets the image, then emit it
# after run() so the same packet can still include the resulting BBox.
old_run = '''            auto& faces = detector->run(image);\n            if (last_stack_log_at == 0 || now - last_stack_log_at >= 30000) {\n'''
new_run = '''            const bool debug_due = last_debug_frame_at == 0 || now - last_debug_frame_at >= kDebugIntervalMs;\n            if (debug_due) {\n                for (int y = 0; y < kDebugHeight; ++y) {\n                    const int source_y = y * height / kDebugHeight;\n                    for (int x = 0; x < kDebugWidth; ++x) {\n                        const int source_x = x * width / kDebugWidth;\n                        debug_gray[y * kDebugWidth + x] = sample_luma(frame, width, source_x, source_y, format);\n                    }\n                }\n            }\n\n            auto& faces = detector->run(image);\n            if (last_stack_log_at == 0 || now - last_stack_log_at >= 30000) {\n'''
if old_run in text:
    text = text.replace(old_run, new_run, 1)
elif "const bool debug_due = last_debug_frame_at == 0" not in text:
    raise SystemExit("detector run insertion point not found; refusing to guess")

old_emit = '''            if (last_debug_frame_at == 0 || now - last_debug_frame_at >= kDebugIntervalMs) {\n                emit_debug_frame(frame, width, height, format, faces, debug_gray, debug_b64, debug_b64_capacity,\n                                 debug_sequence++);\n                last_debug_frame_at = now;\n            }\n'''
new_emit = '''            if (debug_due) {\n                emit_debug_frame(frame, width, height, format, faces, debug_gray, debug_b64, debug_b64_capacity,\n                                 debug_sequence++);\n                last_debug_frame_at = now;\n            }\n'''
if old_emit in text:
    text = text.replace(old_emit, new_emit, 1)
elif "if (debug_due) {\n                emit_debug_frame" not in text:
    raise SystemExit("debug emit block not found; refusing to guess")

path.write_text(text)
print(f"Patched {path}")
print("Debug image source: pre-detector frame bytes")
PY

grep -n -A2 'template <typename FaceContainer>' "$vision_file"
grep -n -A12 'const bool debug_due' "$vision_file"

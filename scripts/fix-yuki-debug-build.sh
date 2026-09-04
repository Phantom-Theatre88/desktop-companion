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
old = "void emit_debug_frame(const uint8_t* frame, int width, int height, int format, const std::vector<dl::detect::result_t>& faces,\n"
new = "template <typename FaceContainer>\nvoid emit_debug_frame(const uint8_t* frame, int width, int height, int format, const FaceContainer& faces,\n"
if old in text:
    text = text.replace(old, new, 1)
elif new not in text:
    raise SystemExit("emit_debug_frame signature not found; refusing to guess")
path.write_text(text)
print(f"Patched {path}")
PY

grep -n -A2 'template <typename FaceContainer>' "$vision_file"

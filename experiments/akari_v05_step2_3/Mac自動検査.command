#!/bin/bash
# AKARI v0.5 STEP4 smoke test. Does not change project files or LOCKs.
set -u
cd "$(dirname "$0")" || exit 1
mkdir -p logs
LOG="logs/akari_smoke_$(date '+%Y%m%d_%H%M%S').log"
exec > >(tee "$LOG") 2>&1
printf 'AKARI v0.5 STEP4 automatic smoke test\nLog: %s\n' "$LOG"
if ! command -v python3 >/dev/null 2>&1; then
  echo 'ERROR: python3 not found'; exit 1
fi
if [ ! -x .venv/bin/python3 ]; then
  python3 -m venv .venv || { echo 'ERROR: unable to create Python environment'; exit 1; }
fi
PY=.venv/bin/python3
"$PY" -c 'import websockets' >/dev/null 2>&1 || "$PY" -m pip install 'websockets>=12,<17' || { echo 'ERROR: unable to install websockets'; exit 1; }
"$PY" -m unittest discover -s ../tests -v || { echo 'FAIL: Python tests'; exit 1; }
GODOT="$(command -v godot || true)"
if [ -z "$GODOT" ] && [ -x /Applications/Godot.app/Contents/MacOS/Godot ]; then
  GODOT=/Applications/Godot.app/Contents/MacOS/Godot
fi
if [ -z "$GODOT" ]; then
  echo 'SKIP: Godot 4.3+ not detected. Python tests are still valid.'
  exit 0
fi
"$GODOT" --version
"$GODOT" --headless --path "$(pwd)" --editor --quit || { echo 'FAIL: Godot import/parse'; exit 1; }
"$GODOT" --headless --path "$(pwd)" --quit-after 300 || { echo 'FAIL: Godot headless scene'; exit 1; }
echo 'PASS: Godot headless load. This does NOT prove visual naturalness, foot contact or live Ghost integration.'
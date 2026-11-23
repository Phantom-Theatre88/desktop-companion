#!/bin/bash
# Strict Godot GLB check. A parseable scene is NOT enough for PASS.
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p logs
LOG="logs/akari_smoke_$(date '+%Y%m%d_%H%M%S').log"
exec > >(tee "$LOG") 2>&1
if ! command -v python3 >/dev/null 2>&1; then echo 'FAIL: python3 not found'; exit 1; fi
if [ ! -x .venv/bin/python3 ]; then python3 -m venv .venv; fi
PY=.venv/bin/python3
"$PY" -c 'import websockets' >/dev/null 2>&1 || "$PY" -m pip install 'websockets>=12,<17'
"$PY" -m unittest discover -s ../tests -v
echo "CHECK: Installing published CC0 motions onto copied AKARI model"
"$PY" tools/install_cc0_motion.py
GODOT="$(command -v godot || true)"
if [ -z "$GODOT" ] && [ -x /Applications/Godot.app/Contents/MacOS/Godot ]; then GODOT=/Applications/Godot.app/Contents/MacOS/Godot; fi
if [ -z "$GODOT" ]; then echo 'FAIL: Godot not found (model load unverified)'; exit 1; fi
"$GODOT" --version
"$GODOT" --headless --path "$(pwd)" --editor --import
echo 'CHECK: validating GLB resource actual mesh/skeleton/morph binding'
"$GODOT" --headless --path "$(pwd)" --script res://tools/check_akari_model.gd
"$GODOT" --headless --path "$(pwd)" --quit-after 30
printf 'PASS: model, skeleton, face morph and motion clips load. Natural motion/contacts remain visually unverified.\n'
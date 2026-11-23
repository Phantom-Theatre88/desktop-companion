#!/bin/bash
set -e
cd "$(dirname "$0")"

if ! command -v python3 >/dev/null 2>&1; then
  echo "Python 3 が必要です。"
  read -r -p "Enterで終了" _
  exit 1
fi

if [ ! -d .venv ]; then
  python3 -m venv .venv
  .venv/bin/python3 -m pip install 'websockets>=12,<17'
fi

GODOT_CMD="$(command -v godot || true)"
if [ -z "$GODOT_CMD" ] && [ -x /Applications/Godot.app/Contents/MacOS/Godot ]; then
  GODOT_CMD=/Applications/Godot.app/Contents/MacOS/Godot
fi
if [ -z "$GODOT_CMD" ]; then
  echo "Godotの実行ファイルが見つかりません。Godot 4.3以上を導入しPATHへ登録してください。"
  read -r -p "Enterで終了" _
  exit 1
fi

# Experimental durable Ghost Memory ledger in this extracted project folder.
# This persists observed results only, not a fabricated emotional baseline.
mkdir -p ghost_server/state
.venv/bin/python3 ghost_server/intent_server.py --headless --journal ghost_server/state/experiences.sqlite &
GHOST_PID=$!
trap 'kill "$GHOST_PID" 2>/dev/null || true' EXIT INT TERM
sleep 1
"$GODOT_CMD" --path "$(pwd)"
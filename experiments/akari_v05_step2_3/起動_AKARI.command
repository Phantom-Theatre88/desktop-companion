#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"

# Godot GLB is an imported resource; extracted ZIP does NOT contain the local
# .godot/imported cache. A first game run without editor import returns null.
GODOT_CMD="$(command -v godot || true)"
if [ -z "$GODOT_CMD" ] && [ -x /Applications/Godot.app/Contents/MacOS/Godot ]; then
  GODOT_CMD=/Applications/Godot.app/Contents/MacOS/Godot
fi
if [ -z "$GODOT_CMD" ]; then
  echo 'FAIL: Godotの実行ファイルが見つかりません。'
  read -r -p 'Enterで終了' _ || true
  exit 1
fi
mkdir -p logs
LOG="logs/akari_start_$(date '+%Y%m%d_%H%M%S').log"
exec > >(tee "$LOG") 2>&1
printf 'AKARI v0.5 model validation start\nLog: %s\n' "$LOG"

# Mandatory import on THIS extraction directory, even if the smoke test ran
# successfully in another extracted copy. Exit on failures; never show a fake model.
if ! command -v python3 >/dev/null 2>&1; then
  echo 'FAIL: Python 3が見つかりません。'; read -r -p 'Enterで終了' _ || true; exit 1
fi
echo '[0/3] CC0人体モーション取得とAKARI骨格への適用'
if ! python3 tools/install_cc0_motion.py; then
  echo 'FAIL: モーション素材取得またはAKARIへの適用に失敗。従来の不自然な歩行は起動しません。'
  read -r -p 'Enterで終了' _ || true
  exit 1
fi
echo '[1/3] GLBインポート開始'
if ! "$GODOT_CMD" --headless --path "$(pwd)" --editor --import; then
  echo 'FAIL: GodotのGLBインポートが失敗しました。'; read -r -p 'Enterで終了' _ || true; exit 1
fi
echo '[2/3] AKARI実モデルのメッシュ・骨格・表情チェック'
if ! "$GODOT_CMD" --headless --path "$(pwd)" --script res://tools/check_akari_model.gd; then
  echo 'FAIL: AKARI本体の読込検査に失敗。赤い代替モデルでは起動しません。'
  read -r -p 'Enterで終了' _ || true
  exit 1
fi

if ! command -v python3 >/dev/null 2>&1; then
  echo 'FAIL: Python 3が見つかりません。'; read -r -p 'Enterで終了' _ || true; exit 1
fi
if [ ! -x .venv/bin/python3 ]; then
  python3 -m venv .venv || { echo 'FAIL: Python仮想環境を作成できません。'; exit 1; }
fi
if ! .venv/bin/python3 -c 'import websockets' >/dev/null 2>&1; then
  .venv/bin/python3 -m pip install 'websockets>=12,<17' || { echo 'FAIL: websocketsの導入に失敗'; exit 1; }
fi
mkdir -p ghost_server/state
.venv/bin/python3 ghost_server/intent_server.py --headless --auto-life --time-scale 60 --journal ghost_server/state/experiences.sqlite &
GHOST_PID=$!
trap 'kill "$GHOST_PID" 2>/dev/null || true' EXIT INT TERM
sleep 1
echo '[3/3] AKARI本体を起動'
"$GODOT_CMD" --path "$(pwd)"
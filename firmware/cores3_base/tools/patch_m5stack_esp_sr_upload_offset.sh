#!/usr/bin/env bash
set -euo pipefail

ROOT="${HOME}/Library/Arduino15/packages/m5stack/hardware/esp32"

if [[ ! -d "$ROOT" ]]; then
  echo "[ERROR] M5Stack Arduino core not found: $ROOT" >&2
  exit 1
fi

CORE_DIR="$(find "$ROOT" -mindepth 1 -maxdepth 1 -type d | sort -V | tail -n 1)"
BOARDS="$CORE_DIR/boards.txt"

if [[ ! -f "$BOARDS" ]]; then
  echo "[ERROR] boards.txt not found: $BOARDS" >&2
  exit 1
fi

echo "[INFO] M5Stack core: $CORE_DIR"
echo "[INFO] boards.txt: $BOARDS"

python3 - "$BOARDS" <<'PY'
from pathlib import Path
import shutil
import sys

boards = Path(sys.argv[1])
text = boards.read_text(encoding="utf-8")

old_offset = "0xD10000"
new_offset = "0xC10000"

lines = text.splitlines(keepends=True)
changed = 0
out = []

for line in lines:
    lower = line.lower()
    # Only touch the ESP-SR model upload argument. Do not globally rewrite
    # partition addresses or unrelated flash offsets.
    if "srmodels.bin" in lower and old_offset.lower() in lower:
        line = line.replace(old_offset, new_offset).replace(old_offset.lower(), new_offset)
        changed += 1
    out.append(line)

if changed == 0:
    already = [
        line.rstrip()
        for line in lines
        if "srmodels.bin" in line.lower() and new_offset.lower() in line.lower()
    ]
    if already:
        print("[OK] ESP-SR upload offset is already 0xC10000")
        for line in already:
            print("     " + line)
        raise SystemExit(0)

    print("[ERROR] Could not find an srmodels.bin upload line using 0xD10000.")
    print("[INFO] Relevant lines in boards.txt:")
    found = False
    for line in lines:
        if "srmodels" in line.lower() or "esp_sr_16" in line.lower():
            print("     " + line.rstrip())
            found = True
    if not found:
        print("     (none)")
    raise SystemExit(2)

backup = boards.with_suffix(".txt.desktop-companion.bak")
if not backup.exists():
    shutil.copy2(boards, backup)
    print(f"[INFO] Backup: {backup}")

boards.write_text("".join(out), encoding="utf-8")
print(f"[OK] Patched {changed} ESP-SR upload line(s): 0xD10000 -> 0xC10000")

# Verify the patched file contains the desired model upload address.
verified = [
    line.rstrip()
    for line in boards.read_text(encoding="utf-8").splitlines()
    if "srmodels.bin" in line.lower() and "0xc10000" in line.lower()
]
if not verified:
    print("[ERROR] Patch write completed but verification failed.", file=sys.stderr)
    raise SystemExit(3)

for line in verified:
    print("     " + line)
PY

echo
echo "[NEXT] Restart Arduino IDE completely, keep:"
echo "       Tools > Partition Scheme > ESP SR 16M"
echo "       then build/upload cores3_base.ino again."

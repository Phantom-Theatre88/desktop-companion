#!/usr/bin/env bash
set -euo pipefail

# Installs the official ESP-SR WakeNet9 "Computer" model into the M5Stack
# Arduino SDK's ESP-SR srmodels.bin, then writes a local (gitignored) marker so
# the firmware is allowed to emit WAKE_WORD_DETECTED word=computer.
#
# This does NOT change Ghost/Heart/Behavior. It only changes the acoustic model.

MODEL_NAME="wn9_computer_tts"
ESP_SR_COMMIT="75dc1efaf76c2253c918962269578e11c348b8f4"
CACHE_ROOT="${HOME}/.cache/desktop-companion"
ESP_SR_DIR="${CACHE_ROOT}/esp-sr"
BUILD_DIR="${CACHE_ROOT}/computer-wakenet-build"
SDKCONFIG="${BUILD_DIR}/sdkconfig"
LOCAL_CONFIG="$(cd "$(dirname "$0")/.." && pwd)/src/config/WakeWordModelLocal.h"

ARDUINO_ROOT="${HOME}/Library/Arduino15/packages/m5stack"

if [[ ! -d "${ARDUINO_ROOT}" ]]; then
  echo "[ERROR] M5Stack Arduino package not found: ${ARDUINO_ROOT}" >&2
  exit 1
fi

mkdir -p "${CACHE_ROOT}"
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"

echo "[1/5] Fetching official ESP-SR model source..."
if [[ ! -d "${ESP_SR_DIR}/.git" ]]; then
  git clone --filter=blob:none https://github.com/espressif/esp-sr.git "${ESP_SR_DIR}"
fi
git -C "${ESP_SR_DIR}" fetch --quiet origin "${ESP_SR_COMMIT}"
git -C "${ESP_SR_DIR}" checkout --quiet --detach "${ESP_SR_COMMIT}"

MODEL_DIR="${ESP_SR_DIR}/model/wakenet_model/${MODEL_NAME}"
if [[ ! -f "${MODEL_DIR}/_MODEL_INFO_" ]]; then
  echo "[ERROR] Official model not found: ${MODEL_DIR}" >&2
  exit 2
fi

echo "[2/5] Building srmodels.bin with only ${MODEL_NAME}..."
cat > "${SDKCONFIG}" <<'EOF'
CONFIG_IDF_TARGET="esp32s3"
CONFIG_SR_WN_WN9_COMPUTER_TTS=y
EOF

python3 "${ESP_SR_DIR}/model/movemodel.py" \
  -d1 "${SDKCONFIG}" \
  -d2 "${ESP_SR_DIR}" \
  -d3 "${BUILD_DIR}"

GENERATED="${BUILD_DIR}/srmodels/srmodels.bin"
if [[ ! -f "${GENERATED}" ]]; then
  echo "[ERROR] srmodels.bin was not generated." >&2
  exit 3
fi

SIZE="$(stat -f%z "${GENERATED}")"
MAX_SIZE=$((0x3E0000))
echo "[INFO] Generated size: ${SIZE} bytes"
if (( SIZE > MAX_SIZE )); then
  echo "[ERROR] Model binary exceeds the project model partition (0x3E0000)." >&2
  exit 4
fi

echo "[3/5] Locating M5Stack Arduino ESP-SR SDK model..."
MODEL_BINS=()
while IFS= read -r model_bin; do
  MODEL_BINS+=("${model_bin}")
done < <(
  find "${ARDUINO_ROOT}/tools" -type f \
    -path '*/esp32s3/esp_sr/srmodels.bin' 2>/dev/null | sort
)

if (( ${#MODEL_BINS[@]} == 0 )); then
  echo "[ERROR] Could not find the installed M5Stack esp32s3/esp_sr/srmodels.bin." >&2
  exit 5
fi

if (( ${#MODEL_BINS[@]} > 1 )); then
  echo "[INFO] Multiple ESP-SR SDK model binaries found:"
  printf '       %s\n' "${MODEL_BINS[@]}"
  echo "[INFO] Installing into all detected ESP32-S3 SDK copies so Arduino's selected package cannot silently use the old model."
fi

echo "[4/5] Backing up and installing Computer model..."
for DEST in "${MODEL_BINS[@]}"; do
  BACKUP="${DEST}.desktop-companion.bak"
  if [[ ! -f "${BACKUP}" ]]; then
    cp -p "${DEST}" "${BACKUP}"
    echo "[INFO] Backup: ${BACKUP}"
  fi
  cp -f "${GENERATED}" "${DEST}"
  cmp -s "${GENERATED}" "${DEST}" || {
    echo "[ERROR] Verification failed after copying to: ${DEST}" >&2
    exit 6
  }
  echo "[OK] Installed: ${DEST}"
done

echo "[5/5] Enabling semantic delivery for the matching local model..."
cat > "${LOCAL_CONFIG}" <<EOF
#pragma once
#define DESKBOT_WAKENET_MODEL_INSTALLED 1
EOF

echo
echo "[OK] WakeNet test model installed:"
echo "     Wake word : Computer"
echo "     Model     : ${MODEL_NAME}"
echo "     Semantic  : word=computer"
echo
echo "[NEXT] Completely restart Arduino IDE."
echo "       Keep Tools > Partition Scheme > ESP SR 16M."
echo "       Then build/upload cores3_base.ino."
echo "       Expected boot line includes:"
echo "       Wake word target: computer model=${MODEL_NAME} ... semantic=READY"

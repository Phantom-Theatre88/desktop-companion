#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
vision_file="$repo_root/build/stackchan/firmware/main/stackchan/vision/yuki_vision.cpp"
header_file="$repo_root/build/stackchan/firmware/main/stackchan/vision/yuki_face_selftest_image.h"

if [[ ! -f "$vision_file" ]]; then
  echo "Generated firmware source not found: $vision_file" >&2
  echo "Run ./scripts/prepare-firmware.sh and ./scripts/fix-yuki-debug-build.sh first." >&2
  exit 1
fi

url="https://raw.githubusercontent.com/espressif/esp-dl/5d9c36063dddbe98b5387828c831d6bbadb1370f/examples/human_face_detect/main/human_face.jpg"
tmp_jpg="$(mktemp -t yuki-face-selftest.XXXXXX.jpg)"
trap 'rm -f "$tmp_jpg"' EXIT

echo "Downloading Espressif official human_face.jpg ..."
curl -fsSL "$url" -o "$tmp_jpg"

python3 - "$tmp_jpg" "$header_file" <<'PY'
from pathlib import Path
import sys
src = Path(sys.argv[1]).read_bytes()
out = Path(sys.argv[2])
lines = ["#pragma once", "#include <cstddef>", "#include <cstdint>", "", "namespace stackchan {", "namespace {", "inline constexpr uint8_t kYukiFaceSelfTestJpeg[] = {"]
for i in range(0, len(src), 16):
    chunk = src[i:i+16]
    lines.append("    " + ", ".join(f"0x{b:02x}" for b in chunk) + ",")
lines += ["};", f"inline constexpr size_t kYukiFaceSelfTestJpegSize = {len(src)};", "}  // namespace", "}  // namespace stackchan", ""]
out.write_text("\n".join(lines))
print(f"Embedded official image: {len(src)} bytes -> {out}")
PY

python3 - "$vision_file" <<'PY'
from pathlib import Path
import sys
p = Path(sys.argv[1])
t = p.read_text()

if '#include "dl_image_jpeg.hpp"' not in t:
    t = t.replace('#include <human_face_detect.hpp>\n', '#include <human_face_detect.hpp>\n#include "dl_image_jpeg.hpp"\n#include "yuki_face_selftest_image.h"\n', 1)

marker = 'class WaveDetector {'
helper = r'''void run_official_face_selftest(HumanFaceDetect& detector)
{
    ESP_LOGI(kTag, "FACE_SELFTEST: starting Espressif official human_face.jpg test");
    dl::image::jpeg_img_t jpeg_img = {
        .data = const_cast<uint8_t*>(kYukiFaceSelfTestJpeg),
        .data_len = kYukiFaceSelfTestJpegSize,
    };
    auto img = dl::image::sw_decode_jpeg(jpeg_img, dl::image::DL_IMAGE_PIX_TYPE_RGB888);
    if (img.data == nullptr || img.width == 0 || img.height == 0) {
        ESP_LOGE(kTag, "FACE_SELFTEST: FAIL jpeg decode");
        if (img.data) heap_caps_free(img.data);
        return;
    }

    auto& results = detector.run(img);
    ESP_LOGI(kTag, "FACE_SELFTEST: decoded %ux%u, results=%u",
             static_cast<unsigned>(img.width), static_cast<unsigned>(img.height),
             static_cast<unsigned>(results.size()));
    float best_score = -1.0f;
    int best_x1 = -1, best_y1 = -1, best_x2 = -1, best_y2 = -1;
    for (const auto& res : results) {
        if (res.box.size() >= 4) {
            ESP_LOGI(kTag, "FACE_SELFTEST: score=%.4f box=(%d,%d)-(%d,%d)",
                     static_cast<double>(res.score), res.box[0], res.box[1], res.box[2], res.box[3]);
            if (res.score > best_score) {
                best_score = res.score;
                best_x1 = res.box[0]; best_y1 = res.box[1];
                best_x2 = res.box[2]; best_y2 = res.box[3];
            }
        }
    }
    if (best_score >= 0.0f) {
        ESP_LOGI(kTag, "FACE_SELFTEST: PASS best score=%.4f box=(%d,%d)-(%d,%d)",
                 static_cast<double>(best_score), best_x1, best_y1, best_x2, best_y2);
    } else {
        ESP_LOGE(kTag, "FACE_SELFTEST: FAIL no face detected in Espressif official test image");
    }
    heap_caps_free(img.data);
}

'''
if 'void run_official_face_selftest(' not in t:
    if marker not in t:
        raise SystemExit('WaveDetector insertion point not found')
    t = t.replace(marker, helper + marker, 1)

old = '    auto detector = std::make_unique<HumanFaceDetect>();\n\n    WaveDetector wave_detector;'
new = '    auto detector = std::make_unique<HumanFaceDetect>();\n    run_official_face_selftest(*detector);\n\n    WaveDetector wave_detector;'
if new not in t:
    if old not in t:
        raise SystemExit('detector construction insertion point not found')
    t = t.replace(old, new, 1)

p.write_text(t)
print(f"Patched {p}")
PY

grep -n 'FACE_SELFTEST' "$vision_file"
echo
echo "Prepared one-shot official face detector self-test."
echo "Next: idf.py build && idf.py -p /dev/cu.usbmodem1101 flash && idf.py -p /dev/cu.usbmodem1101 monitor"

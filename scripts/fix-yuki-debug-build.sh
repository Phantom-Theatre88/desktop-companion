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

# 2) Explicit camera-format -> RGB888 conversion. This mirrors Espressif's
# official examples, which pass an RGB888 dl::image::img_t to HumanFaceDetect.
marker = '''dl::image::pix_type_t to_dl_pixel_type(int format)\n{\n    switch (format) {\n        case V4L2_PIX_FMT_YUYV:\n            return dl::image::DL_IMAGE_PIX_TYPE_YUYV;\n        case V4L2_PIX_FMT_GREY:\n            return dl::image::DL_IMAGE_PIX_TYPE_GRAY;\n        case V4L2_PIX_FMT_RGB565:\n            return dl::image::DL_IMAGE_PIX_TYPE_RGB565LE;\n        case V4L2_PIX_FMT_RGB24:\n        default:\n            return dl::image::DL_IMAGE_PIX_TYPE_RGB888;\n    }\n}\n'''
conversion = r'''dl::image::pix_type_t to_dl_pixel_type(int format)
{
    switch (format) {
        case V4L2_PIX_FMT_YUYV:
            return dl::image::DL_IMAGE_PIX_TYPE_YUYV;
        case V4L2_PIX_FMT_GREY:
            return dl::image::DL_IMAGE_PIX_TYPE_GRAY;
        case V4L2_PIX_FMT_RGB565:
            return dl::image::DL_IMAGE_PIX_TYPE_RGB565LE;
        case V4L2_PIX_FMT_RGB24:
        default:
            return dl::image::DL_IMAGE_PIX_TYPE_RGB888;
    }
}

uint8_t clamp_u8(int value)
{
    return static_cast<uint8_t>(std::clamp(value, 0, 255));
}

bool convert_to_rgb888(const uint8_t* src, int width, int height, int format, uint8_t* dst)
{
    const size_t pixels = static_cast<size_t>(width) * static_cast<size_t>(height);

    if (format == V4L2_PIX_FMT_RGB24) {
        memcpy(dst, src, pixels * 3);
        return true;
    }

    if (format == V4L2_PIX_FMT_GREY) {
        for (size_t i = 0; i < pixels; ++i) {
            const uint8_t y = src[i];
            dst[i * 3 + 0] = y;
            dst[i * 3 + 1] = y;
            dst[i * 3 + 2] = y;
        }
        return true;
    }

    if (format == V4L2_PIX_FMT_RGB565) {
        const auto* p = reinterpret_cast<const uint16_t*>(src);
        for (size_t i = 0; i < pixels; ++i) {
            const uint16_t v = p[i];
            dst[i * 3 + 0] = static_cast<uint8_t>(((v >> 11) & 0x1f) * 255 / 31);
            dst[i * 3 + 1] = static_cast<uint8_t>(((v >> 5) & 0x3f) * 255 / 63);
            dst[i * 3 + 2] = static_cast<uint8_t>((v & 0x1f) * 255 / 31);
        }
        return true;
    }

    if (format == V4L2_PIX_FMT_YUYV) {
        for (size_t i = 0; i < pixels; i += 2) {
            const size_t s = i * 2;
            const int y0 = src[s + 0];
            const int u  = src[s + 1] - 128;
            const int y1 = src[s + 2];
            const int v  = src[s + 3] - 128;

            auto write_pixel = [&](size_t pixel_index, int y) {
                const int c = std::max(0, y - 16);
                const int r = (298 * c + 409 * v + 128) >> 8;
                const int g = (298 * c - 100 * u - 208 * v + 128) >> 8;
                const int b = (298 * c + 516 * u + 128) >> 8;
                dst[pixel_index * 3 + 0] = clamp_u8(r);
                dst[pixel_index * 3 + 1] = clamp_u8(g);
                dst[pixel_index * 3 + 2] = clamp_u8(b);
            };

            write_pixel(i, y0);
            if (i + 1 < pixels) {
                write_pixel(i + 1, y1);
            }
        }
        return true;
    }

    return false;
}
'''
if "bool convert_to_rgb888(" not in text:
    if marker not in text:
        raise SystemExit("pixel conversion insertion point not found; refusing to guess")
    text = text.replace(marker, conversion, 1)

# 3) Mac preview is sampled from the normalized RGB888 detector buffer.
old_sampling = '''{\n    for (int y = 0; y < kDebugHeight; ++y) {\n        const int source_y = y * height / kDebugHeight;\n        for (int x = 0; x < kDebugWidth; ++x) {\n            const int source_x = x * width / kDebugWidth;\n            debug_gray[y * kDebugWidth + x] = sample_luma(frame, width, source_x, source_y, format);\n        }\n    }\n\n    int x1 = -1;\n'''
new_sampling = '''{\n    // debug_gray was captured from the normalized RGB888 detector input.\n    (void)frame;\n    (void)format;\n\n    int x1 = -1;\n'''
if old_sampling in text:
    text = text.replace(old_sampling, new_sampling, 1)
elif "normalized RGB888 detector input" not in text and "debug_gray was captured before detector->run(image)" in text:
    text = text.replace("debug_gray was captured before detector->run(image)",
                        "debug_gray was captured from the normalized RGB888 detector input", 1)
elif "normalized RGB888 detector input" not in text:
    raise SystemExit("emit_debug_frame sampling block not found; refusing to guess")

# 4) Allocate a dedicated RGB888 detector buffer in PSRAM.
old_alloc = '''    auto* frame = static_cast<uint8_t*>(heap_caps_malloc(kMaxFrameBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));\n    auto* debug_gray = static_cast<uint8_t*>(heap_caps_malloc(kDebugWidth * kDebugHeight, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));\n'''
new_alloc = '''    auto* frame = static_cast<uint8_t*>(heap_caps_malloc(kMaxFrameBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));\n    auto* detector_rgb888 = static_cast<uint8_t*>(heap_caps_malloc(kMaxFrameBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));\n    auto* debug_gray = static_cast<uint8_t*>(heap_caps_malloc(kDebugWidth * kDebugHeight, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));\n'''
if "auto* detector_rgb888" not in text:
    if old_alloc not in text:
        raise SystemExit("vision buffer allocation block not found; refusing to guess")
    text = text.replace(old_alloc, new_alloc, 1)

old_check = '''    if (frame == nullptr || debug_gray == nullptr || debug_b64 == nullptr) {\n        ESP_LOGE(kTag, "Unable to allocate vision/debug frame buffers");\n        if (frame) heap_caps_free(frame);\n        if (debug_gray) heap_caps_free(debug_gray);\n'''
new_check = '''    if (frame == nullptr || detector_rgb888 == nullptr || debug_gray == nullptr || debug_b64 == nullptr) {\n        ESP_LOGE(kTag, "Unable to allocate vision/RGB888/debug frame buffers");\n        if (frame) heap_caps_free(frame);\n        if (detector_rgb888) heap_caps_free(detector_rgb888);\n        if (debug_gray) heap_caps_free(debug_gray);\n'''
if "Unable to allocate vision/RGB888/debug frame buffers" not in text:
    if old_check not in text:
        raise SystemExit("vision allocation check block not found; refusing to guess")
    text = text.replace(old_check, new_check, 1)

# 5) Normalize each capture to RGB888 and pass that img_t to HumanFaceDetect.
old_image = '''            dl::image::img_t image = {\n                .data = frame,\n                .width = static_cast<uint16_t>(width),\n                .height = static_cast<uint16_t>(height),\n                .pix_type = to_dl_pixel_type(format),\n            };\n'''
new_image = '''            if (!convert_to_rgb888(frame, width, height, format, detector_rgb888)) {\n                ESP_LOGW(kTag, "Unsupported camera format for RGB888 normalization: 0x%08x", format);\n                vTaskDelay(pdMS_TO_TICKS(kCaptureIntervalMs));\n                continue;\n            }\n\n            dl::image::img_t image = {\n                .data = detector_rgb888,\n                .width = static_cast<uint16_t>(width),\n                .height = static_cast<uint16_t>(height),\n                .pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB888,\n            };\n'''
if ".data = detector_rgb888" not in text:
    if old_image not in text:
        raise SystemExit("detector image block not found; refusing to guess")
    text = text.replace(old_image, new_image, 1)

old_run = '''            auto& faces = detector->run(image);\n            if (last_stack_log_at == 0 || now - last_stack_log_at >= 30000) {\n'''
old_pre_debug = '''            const bool debug_due = last_debug_frame_at == 0 || now - last_debug_frame_at >= kDebugIntervalMs;\n            if (debug_due) {\n                for (int y = 0; y < kDebugHeight; ++y) {\n                    const int source_y = y * height / kDebugHeight;\n                    for (int x = 0; x < kDebugWidth; ++x) {\n                        const int source_x = x * width / kDebugWidth;\n                        debug_gray[y * kDebugWidth + x] = sample_luma(frame, width, source_x, source_y, format);\n                    }\n                }\n            }\n\n            auto& faces = detector->run(image);\n            if (last_stack_log_at == 0 || now - last_stack_log_at >= 30000) {\n'''
new_run = '''            const bool debug_due = last_debug_frame_at == 0 || now - last_debug_frame_at >= kDebugIntervalMs;\n            if (debug_due) {\n                for (int y = 0; y < kDebugHeight; ++y) {\n                    const int source_y = y * height / kDebugHeight;\n                    for (int x = 0; x < kDebugWidth; ++x) {\n                        const int source_x = x * width / kDebugWidth;\n                        const size_t p = (static_cast<size_t>(source_y) * width + source_x) * 3;\n                        const int r = detector_rgb888[p + 0];\n                        const int g = detector_rgb888[p + 1];\n                        const int b = detector_rgb888[p + 2];\n                        debug_gray[y * kDebugWidth + x] = static_cast<uint8_t>((r * 77 + g * 150 + b * 29) >> 8);\n                    }\n                }\n            }\n\n            auto& faces = detector->run(image);\n            if (last_stack_log_at == 0 || now - last_stack_log_at >= 30000) {\n'''
if "const int r = detector_rgb888[p + 0]" not in text:
    if old_pre_debug in text:
        text = text.replace(old_pre_debug, new_run, 1)
    elif old_run in text:
        text = text.replace(old_run, new_run, 1)
    else:
        raise SystemExit("detector run insertion point not found; refusing to guess")

old_emit = '''            if (last_debug_frame_at == 0 || now - last_debug_frame_at >= kDebugIntervalMs) {\n                emit_debug_frame(frame, width, height, format, faces, debug_gray, debug_b64, debug_b64_capacity,\n                                 debug_sequence++);\n                last_debug_frame_at = now;\n            }\n'''
new_emit = '''            if (debug_due) {\n                emit_debug_frame(detector_rgb888, width, height, V4L2_PIX_FMT_RGB24, faces, debug_gray, debug_b64, debug_b64_capacity,\n                                 debug_sequence++);\n                last_debug_frame_at = now;\n            }\n'''
if old_emit in text:
    text = text.replace(old_emit, new_emit, 1)
else:
    old_emit_previous = '''            if (debug_due) {\n                emit_debug_frame(frame, width, height, format, faces, debug_gray, debug_b64, debug_b64_capacity,\n                                 debug_sequence++);\n                last_debug_frame_at = now;\n            }\n'''
    if old_emit_previous in text:
        text = text.replace(old_emit_previous, new_emit, 1)
    elif "emit_debug_frame(detector_rgb888" not in text:
        raise SystemExit("debug emit block not found; refusing to guess")

# 6) FINAL A-test: follow the detector's own confidence score. The previous
# implementation selected the largest box, which can lock onto a large false
# positive (such as the shelf edge) even when a better-scoring face exists.
# Espressif's result_t exposes res.score in its official examples.
old_cmp = '''[](const auto& left, const auto& right) {\n                    return left.box_area() < right.box_area();\n                }'''
new_cmp = '''[](const auto& left, const auto& right) {\n                    return left.score < right.score;\n                }'''
text = text.replace(old_cmp, new_cmp)

old_cmp_debug = '''[](const auto& left, const auto& right) {\n            return left.box_area() < right.box_area();\n        }'''
new_cmp_debug = '''[](const auto& left, const auto& right) {\n            return left.score < right.score;\n        }'''
text = text.replace(old_cmp_debug, new_cmp_debug)

if "return left.box_area() < right.box_area();" in text:
    raise SystemExit("largest-box face selection still remains; refusing to continue")
if "return left.score < right.score;" not in text:
    raise SystemExit("score-based face selection was not installed")

path.write_text(text)
print(f"Patched {path}")
print("Face detector input: explicit RGB888")
print("Face selection: highest detector score (official result_t score)")
print("Mac debug image source: same RGB888 detector buffer")
PY

grep -n -A2 'template <typename FaceContainer>' "$vision_file"
grep -n -A6 'convert_to_rgb888' "$vision_file"
grep -n -A10 '.data = detector_rgb888' "$vision_file"
grep -n -A2 'return left.score < right.score' "$vision_file"

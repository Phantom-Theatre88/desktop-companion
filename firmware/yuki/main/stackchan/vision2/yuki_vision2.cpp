/*
 * YukiVision2
 *
 * Independent, deliberately small person-detection pipeline:
 *   camera -> canonical RGB888 -> PedestrianDetect -> person observation
 *
 * The legacy YukiVision face detector is not used here.  Keeping the two paths
 * separate makes it possible to A/B test them on the same hardware and remove
 * the old path only after the new one is proven on-device.
 */
#include "yuki_vision2.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>

#include <esp_heap_caps.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <linux/videodev2.h>
#include <pedestrian_detect.hpp>

#include <hal/board/hal_bridge.h>
#include <hal/board/stackchan_camera.h>
#include <hal/hal.h>

namespace stackchan {
namespace {

constexpr char kTag[] = "YukiVision2";
constexpr int kMaxWidth = 320;
constexpr int kMaxHeight = 240;
constexpr size_t kMaxRgbBytes = static_cast<size_t>(kMaxWidth) * kMaxHeight * 3;
constexpr uint32_t kCaptureIntervalMs = 220;
constexpr uint32_t kStartupDelayMs = 1500;
constexpr uint32_t kPersonTimeoutMs = 1200;

std::atomic<bool> vision2_started{false};
std::atomic<bool> vision2_enabled{false};

std::atomic<int> obs_frame_width{0};
std::atomic<int> obs_frame_height{0};
std::atomic<int> obs_x1{-1};
std::atomic<int> obs_y1{-1};
std::atomic<int> obs_x2{-1};
std::atomic<int> obs_y2{-1};
std::atomic<int> obs_center_x{-1};
std::atomic<int> obs_center_y{-1};
std::atomic<uint32_t> obs_confidence_milli{0};
std::atomic<uint32_t> obs_seen_at{0};

inline uint8_t clamp_byte(int value)
{
    return static_cast<uint8_t>(std::clamp(value, 0, 255));
}

size_t expected_source_bytes(int width, int height, int format)
{
    if (width <= 0 || height <= 0) {
        return 0;
    }
    const size_t pixels = static_cast<size_t>(width) * height;
    switch (format) {
        case V4L2_PIX_FMT_GREY:
            return pixels;
        case V4L2_PIX_FMT_YUYV:
        case V4L2_PIX_FMT_RGB565:
            return pixels * 2;
        case V4L2_PIX_FMT_RGB24:
            return pixels * 3;
        default:
            return 0;
    }
}

bool convert_to_rgb888(const uint8_t* source,
                       size_t source_length,
                       int width,
                       int height,
                       int format,
                       uint8_t* rgb,
                       size_t rgb_capacity)
{
    if (source == nullptr || rgb == nullptr || width <= 0 || height <= 0) {
        return false;
    }

    const size_t pixels = static_cast<size_t>(width) * height;
    const size_t rgb_bytes = pixels * 3;
    const size_t expected = expected_source_bytes(width, height, format);
    if (rgb_bytes > rgb_capacity || expected == 0 || source_length < expected) {
        return false;
    }

    if (format == V4L2_PIX_FMT_RGB24) {
        std::memcpy(rgb, source, rgb_bytes);
        return true;
    }

    if (format == V4L2_PIX_FMT_GREY) {
        for (size_t i = 0; i < pixels; ++i) {
            const uint8_t y = source[i];
            rgb[i * 3] = y;
            rgb[i * 3 + 1] = y;
            rgb[i * 3 + 2] = y;
        }
        return true;
    }

    if (format == V4L2_PIX_FMT_RGB565) {
        for (size_t i = 0; i < pixels; ++i) {
            const uint16_t p = static_cast<uint16_t>(source[i * 2]) |
                               (static_cast<uint16_t>(source[i * 2 + 1]) << 8);
            const uint8_t r5 = static_cast<uint8_t>((p >> 11) & 0x1f);
            const uint8_t g6 = static_cast<uint8_t>((p >> 5) & 0x3f);
            const uint8_t b5 = static_cast<uint8_t>(p & 0x1f);
            rgb[i * 3] = static_cast<uint8_t>((r5 << 3) | (r5 >> 2));
            rgb[i * 3 + 1] = static_cast<uint8_t>((g6 << 2) | (g6 >> 4));
            rgb[i * 3 + 2] = static_cast<uint8_t>((b5 << 3) | (b5 >> 2));
        }
        return true;
    }

    if (format == V4L2_PIX_FMT_YUYV) {
        // StackChan's esp_video path exposes YUV422 as packed YUYV:
        // Y0 U Y1 V. Convert each pair to canonical RGB888 before inference.
        for (size_t i = 0; i + 1 < pixels; i += 2) {
            const size_t src = i * 2;
            const int y0 = source[src];
            const int u = source[src + 1] - 128;
            const int y1 = source[src + 2];
            const int v = source[src + 3] - 128;

            const auto write_pixel = [&](size_t pixel, int y) {
                const int c = std::max(0, y - 16);
                const int r = (298 * c + 409 * v + 128) >> 8;
                const int g = (298 * c - 100 * u - 208 * v + 128) >> 8;
                const int b = (298 * c + 516 * u + 128) >> 8;
                rgb[pixel * 3] = clamp_byte(r);
                rgb[pixel * 3 + 1] = clamp_byte(g);
                rgb[pixel * 3 + 2] = clamp_byte(b);
            };
            write_pixel(i, y0);
            write_pixel(i + 1, y1);
        }
        return true;
    }

    return false;
}

void clear_observation_if_stale(uint32_t now)
{
    const uint32_t seen = obs_seen_at.load();
    if (seen == 0 || now - seen <= kPersonTimeoutMs) {
        return;
    }
    obs_x1.store(-1);
    obs_y1.store(-1);
    obs_x2.store(-1);
    obs_y2.store(-1);
    obs_center_x.store(-1);
    obs_center_y.store(-1);
    obs_confidence_milli.store(0);
}

void publish_person(const dl::detect::result_t& result, int width, int height, uint32_t now)
{
    if (result.box.size() < 4) {
        return;
    }

    const int x1 = std::clamp(result.box[0], 0, width - 1);
    const int y1 = std::clamp(result.box[1], 0, height - 1);
    const int x2 = std::clamp(result.box[2], 0, width - 1);
    const int y2 = std::clamp(result.box[3], 0, height - 1);
    const int center_x = (x1 + x2) / 2;
    const int center_y = (y1 + y2) / 2;
    const float confidence = std::clamp(result.score, 0.0f, 1.0f);

    obs_frame_width.store(width);
    obs_frame_height.store(height);
    obs_x1.store(x1);
    obs_y1.store(y1);
    obs_x2.store(x2);
    obs_y2.store(y2);
    obs_center_x.store(center_x);
    obs_center_y.store(center_y);
    obs_confidence_milli.store(static_cast<uint32_t>(confidence * 1000.0f + 0.5f));
    obs_seen_at.store(now);

    ESP_LOGI(kTag,
             "PERSON score=%.3f box=(%d,%d)-(%d,%d) center=(%d,%d) frame=%dx%d",
             confidence, x1, y1, x2, y2, center_x, center_y, width, height);
}

void yuki_vision2_task(void*)
{
    while (!vision2_enabled.load()) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelay(pdMS_TO_TICKS(kStartupDelayMs));

    auto* camera_frame = static_cast<uint8_t*>(
        heap_caps_malloc(kMaxRgbBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    auto* detector_rgb888 = static_cast<uint8_t*>(
        heap_caps_malloc(kMaxRgbBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

    if (camera_frame == nullptr || detector_rgb888 == nullptr) {
        ESP_LOGE(kTag, "Unable to allocate Vision2 PSRAM frame buffers");
        if (camera_frame) heap_caps_free(camera_frame);
        if (detector_rgb888) heap_caps_free(detector_rgb888);
        vision2_started.store(false);
        vTaskDelete(nullptr);
        return;
    }

    // This is a person/pedestrian detector, not the legacy face detector.
    auto detector = std::make_unique<PedestrianDetect>();
    ESP_LOGI(kTag, "Vision2 ready: camera -> RGB888 -> PedestrianDetect");

    while (true) {
        if (!vision2_enabled.load()) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        auto* camera = hal_bridge::board_get_camera();
        size_t length = 0;
        int width = 0;
        int height = 0;
        int format = 0;

        if (camera != nullptr &&
            camera->CaptureForVision(camera_frame, kMaxRgbBytes, length, width, height, format)) {
            const uint32_t now = GetHAL().millis();

            if (width <= 0 || height <= 0 || width > kMaxWidth || height > kMaxHeight) {
                ESP_LOGW(kTag, "Unsupported frame geometry %dx%d", width, height);
                vTaskDelay(pdMS_TO_TICKS(kCaptureIntervalMs));
                continue;
            }

            const size_t expected = expected_source_bytes(width, height, format);
            if (expected == 0 || length < expected) {
                ESP_LOGW(kTag,
                         "Frame format/length mismatch: format=0x%08x len=%u expected=%u frame=%dx%d",
                         static_cast<unsigned>(format),
                         static_cast<unsigned>(length),
                         static_cast<unsigned>(expected),
                         width,
                         height);
                vTaskDelay(pdMS_TO_TICKS(kCaptureIntervalMs));
                continue;
            }

            if (!convert_to_rgb888(camera_frame,
                                   length,
                                   width,
                                   height,
                                   format,
                                   detector_rgb888,
                                   kMaxRgbBytes)) {
                ESP_LOGW(kTag, "RGB888 conversion failed for format=0x%08x", static_cast<unsigned>(format));
                vTaskDelay(pdMS_TO_TICKS(kCaptureIntervalMs));
                continue;
            }

            // IMPORTANT: bbox coordinates returned below refer directly to this
            // exact width/height/RGB888 buffer. No post-detection rotation,
            // axis swap or inferred camera orientation is applied.
            dl::image::img_t image = {
                .data = detector_rgb888,
                .width = static_cast<uint16_t>(width),
                .height = static_cast<uint16_t>(height),
                .pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB888,
            };

            auto& people = detector->run(image);
            if (!people.empty()) {
                const auto best = std::max_element(
                    people.begin(), people.end(), [](const auto& left, const auto& right) {
                        if (std::abs(left.score - right.score) > 0.01f) {
                            return left.score < right.score;
                        }
                        return left.box_area() < right.box_area();
                    });
                if (best != people.end()) {
                    publish_person(*best, width, height, now);
                }
            } else {
                clear_observation_if_stale(now);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(kCaptureIntervalMs));
    }
}

}  // namespace

void StartYukiVision2()
{
    bool expected = false;
    if (!vision2_started.compare_exchange_strong(expected, true)) {
        return;
    }

    constexpr uint32_t kTaskStackBytes = 12288;
    const BaseType_t result = xTaskCreatePinnedToCore(
        yuki_vision2_task, "yuki_vision2", kTaskStackBytes, nullptr, 2, nullptr, 0);
    if (result != pdPASS) {
        ESP_LOGE(kTag, "Failed to create Vision2 task");
        vision2_started.store(false);
    }
}

void EnableYukiVision2()
{
    vision2_enabled.store(true);
}

void DisableYukiVision2()
{
    vision2_enabled.store(false);
}

YukiPersonObservation GetYukiPersonObservation()
{
    YukiPersonObservation result;
    const uint32_t now = GetHAL().millis();
    const uint32_t seen = obs_seen_at.load();

    result.frame_width = obs_frame_width.load();
    result.frame_height = obs_frame_height.load();
    result.x1 = obs_x1.load();
    result.y1 = obs_y1.load();
    result.x2 = obs_x2.load();
    result.y2 = obs_y2.load();
    result.center_x = obs_center_x.load();
    result.center_y = obs_center_y.load();
    result.confidence = static_cast<float>(obs_confidence_milli.load()) / 1000.0f;
    result.seen_at_ms = seen;
    result.detected = seen != 0 && now - seen <= kPersonTimeoutMs && result.center_x >= 0 && result.center_y >= 0;
    return result;
}

bool YukiPersonSeenRecently(uint32_t within_ms)
{
    const uint32_t seen = obs_seen_at.load();
    if (seen == 0) {
        return false;
    }
    return GetHAL().millis() - seen <= within_ms;
}

}  // namespace stackchan

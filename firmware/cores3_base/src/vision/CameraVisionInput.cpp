#include "CameraVisionInput.h"

namespace deskbot {
namespace vision {

void CameraVisionInput::onCameraFrame(const device::CameraFrameView& frame) {
  VisionFrameSummary summary;
  summary.timestamp_ms = frame.timestamp_ms;
  summary.width = frame.width;
  summary.height = frame.height;
  summary.bytes = frame.bytes;
  summary.valid = frame.data != nullptr && frame.bytes >= 2;

  if (summary.valid) {
    summary.average_luma = estimateLumaRgb565(frame.data, frame.bytes);
  }

  last_summary_ = summary;
}

uint8_t CameraVisionInput::estimateLumaRgb565(const uint8_t* data,
                                              size_t bytes) {
  if (data == nullptr || bytes < 2) {
    return 0;
  }

  uint32_t sum = 0;
  uint32_t count = 0;

  // Low-cost low-level visual observation. This remains in the Vision layer,
  // not the Device Driver, so raw camera hardware stays separate from meaning.
  constexpr size_t kStrideBytes = 512;
  for (size_t i = 0; i + 1 < bytes; i += kStrideBytes) {
    const uint16_t pixel = static_cast<uint16_t>(data[i]) |
                           (static_cast<uint16_t>(data[i + 1]) << 8);
    const uint8_t r5 = (pixel >> 11) & 0x1F;
    const uint8_t g6 = (pixel >> 5) & 0x3F;
    const uint8_t b5 = pixel & 0x1F;

    const uint16_t r = (r5 * 255u) / 31u;
    const uint16_t g = (g6 * 255u) / 63u;
    const uint16_t b = (b5 * 255u) / 31u;
    const uint16_t y = static_cast<uint16_t>(
        (r * 30u + g * 59u + b * 11u) / 100u);

    sum += y;
    ++count;
  }

  return count == 0 ? 0 : static_cast<uint8_t>(sum / count);
}

}  // namespace vision
}  // namespace deskbot

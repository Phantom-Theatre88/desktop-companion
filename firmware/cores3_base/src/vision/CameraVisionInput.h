#pragma once

#include <Arduino.h>

#include "../device/CoreS3CameraDriver.h"

namespace deskbot {
namespace vision {

struct VisionFrameSummary {
  bool valid = false;
  uint32_t timestamp_ms = 0;
  uint16_t width = 0;
  uint16_t height = 0;
  size_t bytes = 0;
  uint8_t average_luma = 0;
};

class CameraVisionInput : public device::CameraFrameConsumer {
 public:
  void onCameraFrame(const device::CameraFrameView& frame) override;

  const VisionFrameSummary& lastSummary() const { return last_summary_; }

 private:
  static uint8_t estimateLumaRgb565(const uint8_t* data, size_t bytes);

  VisionFrameSummary last_summary_;
};

}  // namespace vision
}  // namespace deskbot

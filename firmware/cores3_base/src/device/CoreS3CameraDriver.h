#pragma once

#include <Arduino.h>

namespace deskbot {
namespace device {

struct CameraFrameView {
  const uint8_t* data = nullptr;
  size_t bytes = 0;
  uint16_t width = 0;
  uint16_t height = 0;
  uint32_t timestamp_ms = 0;
};

class CameraFrameConsumer {
 public:
  virtual ~CameraFrameConsumer() = default;
  virtual void onCameraFrame(const CameraFrameView& frame) = 0;
};

struct CameraCaptureResult {
  bool initialized = false;
  bool captured = false;
  bool internal_i2c_restored = false;
  uint16_t width = 0;
  uint16_t height = 0;
  size_t bytes = 0;
  uint32_t timestamp_ms = 0;
};

class CoreS3CameraDriver {
 public:
  CameraCaptureResult captureOnce(uint32_t now_ms,
                                  CameraFrameConsumer* consumer = nullptr);
};

}  // namespace device
}  // namespace deskbot

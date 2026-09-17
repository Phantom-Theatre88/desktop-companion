#pragma once

#include <Arduino.h>

namespace deskbot {
namespace device {

struct CameraProbeResult {
  bool initialized = false;
  bool captured = false;
  bool internal_i2c_restored = false;
  uint16_t width = 0;
  uint16_t height = 0;
  size_t bytes = 0;
  uint8_t average_luma = 0;
};

class CoreS3CameraDriver {
 public:
  CameraProbeResult probeOnce();

 private:
  static uint8_t estimateLumaRgb565(const uint8_t* data, size_t bytes);
};

}  // namespace device
}  // namespace deskbot

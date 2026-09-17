#pragma once

#include <Arduino.h>
#include <M5Unified.h>

namespace deskbot {
namespace device {

struct ImuSample {
  bool available = false;
  bool valid = false;
  float ax = 0.0f;
  float ay = 0.0f;
  float az = 0.0f;
  uint32_t timestamp_ms = 0;
};

class CoreS3ImuDriver {
 public:
  void begin();
  ImuSample sample(uint32_t now_ms) const;

  bool available() const { return available_; }

 private:
  bool available_ = false;
};

}  // namespace device
}  // namespace deskbot

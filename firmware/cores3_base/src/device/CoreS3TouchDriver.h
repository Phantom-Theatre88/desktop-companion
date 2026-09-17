#pragma once

#include <Arduino.h>
#include <M5Unified.h>

namespace deskbot {
namespace device {

struct TouchSample {
  bool available = false;
  bool active = false;
  bool pressed = false;
  bool released = false;
  int32_t x = 0;
  int32_t y = 0;
  uint32_t timestamp_ms = 0;
};

class CoreS3TouchDriver {
 public:
  void begin();
  TouchSample sample(uint32_t now_ms);

 private:
  bool previous_active_ = false;
};

}  // namespace device
}  // namespace deskbot

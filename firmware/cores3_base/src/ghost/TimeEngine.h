#pragma once

#include <Arduino.h>

namespace deskbot {
namespace ghost {

class TimeEngine {
 public:
  void begin(uint32_t now_ms) { last_tick_ms_ = now_ms; }
  void tick(uint32_t now_ms) { last_tick_ms_ = now_ms; }

  uint32_t lastTickMs() const { return last_tick_ms_; }

 private:
  uint32_t last_tick_ms_ = 0;
};

}  // namespace ghost
}  // namespace deskbot

#pragma once
#include <Arduino.h>
namespace deskbot { namespace ghost {
using UnixClock = bool (*)(uint64_t& seconds, void* context);
enum class ElapsedStatus : uint8_t { KNOWN, CLOCK_UNKNOWN, CLOCK_REVERSED };
struct OfflineElapsed {
  ElapsedStatus status = ElapsedStatus::CLOCK_UNKNOWN;
  uint64_t seconds = 0;
};
class TimeEngine {
 public:
  void setUnixClock(UnixClock clock, void* context = nullptr) {
    clock_ = clock; clock_context_ = context;
  }
  void begin(uint32_t now_ms) { last_tick_ms_ = now_ms; uptime_ms_ = 0; }
  void tick(uint32_t now_ms) {
    uptime_ms_ += static_cast<uint32_t>(now_ms - last_tick_ms_);
    last_tick_ms_ = now_ms;
  }
  uint32_t lastTickMs() const { return last_tick_ms_; }
  uint64_t uptimeMs() const { return uptime_ms_; }
  uint64_t unixSeconds() const {
    uint64_t seconds = 0;
    if (!clock_ || !clock_(seconds, clock_context_) ||
        seconds < 946684800ULL || seconds > 4102444800ULL) return 0;
    return seconds;
  }
  OfflineElapsed elapsedSince(uint64_t saved) const {
    const uint64_t now = unixSeconds();
    if (!saved || !now) return {};
    if (now < saved) return {ElapsedStatus::CLOCK_REVERSED, 0};
    return {ElapsedStatus::KNOWN, now - saved};
  }
 private:
  UnixClock clock_ = nullptr;
  void* clock_context_ = nullptr;
  uint32_t last_tick_ms_ = 0;
  uint64_t uptime_ms_ = 0;
};
} }

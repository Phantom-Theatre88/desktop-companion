#include "CoreS3TouchDriver.h"

namespace deskbot {
namespace device {

void CoreS3TouchDriver::begin() {
  previous_active_ = false;
}

TouchSample CoreS3TouchDriver::sample(uint32_t now_ms) {
  TouchSample out;
  out.timestamp_ms = now_ms;
  out.available = M5.Touch.isEnabled();

  if (!out.available) {
    previous_active_ = false;
    return out;
  }

  const size_t count = M5.Touch.getCount();
  out.active = count > 0;
  out.pressed = out.active && !previous_active_;
  out.released = !out.active && previous_active_;

  if (out.active) {
    const auto detail = M5.Touch.getDetail(0);
    out.x = detail.x;
    out.y = detail.y;
  }

  previous_active_ = out.active;
  return out;
}

}  // namespace device
}  // namespace deskbot

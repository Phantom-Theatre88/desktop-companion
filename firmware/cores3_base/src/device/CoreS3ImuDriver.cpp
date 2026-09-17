#include "CoreS3ImuDriver.h"

namespace deskbot {
namespace device {

void CoreS3ImuDriver::begin() {
  available_ = M5.Imu.isEnabled();
}

ImuSample CoreS3ImuDriver::sample(uint32_t now_ms) const {
  ImuSample out;
  out.timestamp_ms = now_ms;
  out.available = available_ && M5.Imu.isEnabled();

  if (!out.available) {
    return out;
  }

  out.valid = M5.Imu.getAccelData(&out.ax, &out.ay, &out.az);
  return out;
}

}  // namespace device
}  // namespace deskbot

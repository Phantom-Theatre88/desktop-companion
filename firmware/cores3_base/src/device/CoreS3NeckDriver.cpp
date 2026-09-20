#include "CoreS3NeckDriver.h"

#include <math.h>
#include <M5Unified.h>

namespace deskbot {
namespace device {
namespace {

constexpr uint32_t kServoBaud = 1000000;

// M5StackChan CoreS3 mount board: the SCS servo rail is NOT permanently on.
// The official platform enables VM_EN through PY32 IO expander @ 0x6F, pin 0.
constexpr uint8_t kPy32Address = 0x6F;
constexpr uint32_t kPy32I2cHz = 100000;
constexpr uint8_t kPy32RegVersion = 0x02;
constexpr uint8_t kPy32RegGpioModeL = 0x03;
constexpr uint8_t kPy32RegGpioOutL = 0x05;
constexpr uint8_t kPy32RegGpioPullUpL = 0x09;
constexpr uint8_t kPy32RegGpioPullDownL = 0x0B;
constexpr uint8_t kServoPowerMask = 0x01;  // PY32 pin 0 / VM_EN
constexpr uint32_t kServoPowerSettleMs = 200;
constexpr int kServoRxPin = 7;
constexpr int kServoTxPin = 6;
constexpr uint8_t kYawId = 1;
constexpr uint8_t kPitchId = 2;

constexpr uint8_t kTorqueEnableAddress = 40;
constexpr uint8_t kGoalPositionAddress = 42;
constexpr uint8_t kSyncWriteInstruction = 0x83;
constexpr uint8_t kBroadcastId = 0xFE;

// Official Stack-chan donor defaults. Kept inside the Device Driver boundary.
constexpr int16_t kYawZeroRaw = 460;
constexpr int16_t kPitchZeroRaw = 620;
constexpr int16_t kServoRawMin = 0;
constexpr int16_t kServoRawMax = 1000;

// Deliberately conservative life-motion range for first real-device pass.
constexpr float kYawMaxDegrees = 12.0f;
constexpr float kPitchMaxDegrees = 7.0f;
constexpr float kRawPerDegree = 3.2f;

constexpr uint32_t kUpdateIntervalMs = 50;  // 20Hz body motion.
constexpr uint32_t kVisionSettleAfterMoveMs = 650;
constexpr int16_t kMinRawChange = 2;

}  // namespace

bool CoreS3NeckDriver::begin(uint32_t now_ms) {
  available_ = false;
  torque_enabled_ = false;
  servo_power_ready_ = enableServoPower();
  if (!servo_power_ready_) {
    return false;
  }

  // Give the mount-board servo rail time to rise before speaking SCS protocol.
  delay(kServoPowerSettleMs);
  serial_.begin(kServoBaud, SERIAL_8N1, kServoRxPin, kServoTxPin);
  delay(5);

  available_ = true;
  last_update_ms_ = now_ms;
  last_motion_command_ms_ = now_ms;
  setTorque(true);
  writePositions(kYawZeroRaw, kPitchZeroRaw);
  last_yaw_raw_ = kYawZeroRaw;
  last_pitch_raw_ = kPitchZeroRaw;
  last_motion_command_ = NeckMotionCommand{};
  last_motion_command_.command_ms = now_ms;
  return available_;
}

void CoreS3NeckDriver::tick(uint32_t now_ms,
                            float yaw_norm,
                            float pitch_norm) {
  if (!available_ || (now_ms - last_update_ms_) < kUpdateIntervalMs) {
    return;
  }
  last_update_ms_ = now_ms;

  const int16_t yaw_raw =
      normToRaw(yaw_norm,
                kYawZeroRaw,
                kYawMaxDegrees,
                kServoRawMin,
                kServoRawMax);
  const int16_t pitch_raw =
      normToRaw(pitch_norm,
                kPitchZeroRaw,
                kPitchMaxDegrees,
                kServoRawMin,
                kServoRawMax);

  const int16_t yaw_delta =
      yaw_raw > last_yaw_raw_ ? yaw_raw - last_yaw_raw_ : last_yaw_raw_ - yaw_raw;
  const int16_t pitch_delta =
      pitch_raw > last_pitch_raw_
          ? pitch_raw - last_pitch_raw_
          : last_pitch_raw_ - pitch_raw;

  if (yaw_delta < kMinRawChange && pitch_delta < kMinRawChange) {
    return;
  }

  if (!torque_enabled_) {
    setTorque(true);
  }

  const float yaw_delta_deg =
      static_cast<float>(yaw_raw - last_yaw_raw_) / kRawPerDegree;
  const float pitch_delta_deg =
      static_cast<float>(pitch_raw - last_pitch_raw_) / kRawPerDegree;

  writePositions(yaw_raw, pitch_raw);
  last_motion_command_ms_ = now_ms;

  ++last_motion_command_.sequence;
  last_motion_command_.command_ms = now_ms;
  last_motion_command_.yaw_delta_deg = yaw_delta_deg;
  last_motion_command_.pitch_delta_deg = pitch_delta_deg;

  last_yaw_raw_ = yaw_raw;
  last_pitch_raw_ = pitch_raw;
}

bool CoreS3NeckDriver::motionRecently(uint32_t now_ms) const {
  return available_ &&
         (now_ms - last_motion_command_ms_) < kVisionSettleAfterMoveMs;
}

float CoreS3NeckDriver::clampSigned(float value) {
  if (!isfinite(value)) return 0.0f;
  if (value < -1.0f) return -1.0f;
  if (value > 1.0f) return 1.0f;
  return value;
}

int16_t CoreS3NeckDriver::clampRaw(int16_t value,
                                   int16_t low,
                                   int16_t high) {
  if (value < low) return low;
  if (value > high) return high;
  return value;
}

int16_t CoreS3NeckDriver::normToRaw(float normalized,
                                    int16_t zero_raw,
                                    float max_degrees,
                                    int16_t low_raw,
                                    int16_t high_raw) {
  const float n = clampSigned(normalized);
  const float raw_offset = n * max_degrees * kRawPerDegree;
  const int16_t target =
      static_cast<int16_t>(roundf(static_cast<float>(zero_raw) + raw_offset));
  return clampRaw(target, low_raw, high_raw);
}

bool CoreS3NeckDriver::updatePy32Bit(uint8_t reg,
                                      uint8_t mask,
                                      bool enabled) {
  uint8_t value = 0;
  if (!M5.In_I2C.readRegister(
          kPy32Address, reg, &value, 1, kPy32I2cHz)) {
    return false;
  }
  const uint8_t next =
      enabled ? static_cast<uint8_t>(value | mask)
              : static_cast<uint8_t>(value & ~mask);
  return M5.In_I2C.writeRegister8(
      kPy32Address, reg, next, kPy32I2cHz);
}

bool CoreS3NeckDriver::enableServoPower() {
  if (!M5.In_I2C.isEnabled() && !M5.In_I2C.begin()) {
    return false;
  }

  uint8_t version = 0;
  if (!M5.In_I2C.readRegister(
          kPy32Address, kPy32RegVersion, &version, 1, kPy32I2cHz)) {
    return false;
  }
  servo_power_version_ = version;
  if (version == 0x00 || version == 0xFF) {
    return false;
  }

  // Mirror the official M5StackChan CoreS3 servo-power sequence:
  // pin0 output, pull-up enabled / pull-down disabled, then VM_EN high.
  if (!updatePy32Bit(kPy32RegGpioModeL, kServoPowerMask, true)) return false;
  if (!updatePy32Bit(kPy32RegGpioPullDownL, kServoPowerMask, false)) return false;
  if (!updatePy32Bit(kPy32RegGpioPullUpL, kServoPowerMask, true)) return false;
  if (!updatePy32Bit(kPy32RegGpioOutL, kServoPowerMask, true)) return false;

  uint8_t out = 0;
  if (!M5.In_I2C.readRegister(
          kPy32Address, kPy32RegGpioOutL, &out, 1, kPy32I2cHz)) {
    return false;
  }
  return (out & kServoPowerMask) != 0;
}

void CoreS3NeckDriver::setTorque(bool enabled) {
  uint8_t yaw_data[1] = {static_cast<uint8_t>(enabled ? 1 : 0)};
  uint8_t pitch_data[1] = {static_cast<uint8_t>(enabled ? 1 : 0)};
  syncWrite(kTorqueEnableAddress, 1, yaw_data, pitch_data);
  torque_enabled_ = enabled;
}

void CoreS3NeckDriver::writePositions(int16_t yaw_raw, int16_t pitch_raw) {
  uint8_t yaw_data[6] = {};
  uint8_t pitch_data[6] = {};

  writeWordBE(yaw_data + 0, static_cast<uint16_t>(yaw_raw));
  writeWordBE(yaw_data + 2, 20);  // Same conservative move-time seed as donor HAL.
  writeWordBE(yaw_data + 4, 0);

  writeWordBE(pitch_data + 0, static_cast<uint16_t>(pitch_raw));
  writeWordBE(pitch_data + 2, 20);
  writeWordBE(pitch_data + 4, 0);

  syncWrite(kGoalPositionAddress, 6, yaw_data, pitch_data);
}

void CoreS3NeckDriver::syncWrite(uint8_t address,
                                 uint8_t data_len,
                                 const uint8_t* id1_data,
                                 const uint8_t* id2_data) {
  const uint8_t packet_len =
      static_cast<uint8_t>((data_len + 1) * 2 + 4);

  uint8_t checksum =
      static_cast<uint8_t>(kBroadcastId + packet_len +
                           kSyncWriteInstruction + address + data_len);

  serial_.write(0xFF);
  serial_.write(0xFF);
  serial_.write(kBroadcastId);
  serial_.write(packet_len);
  serial_.write(kSyncWriteInstruction);
  serial_.write(address);
  serial_.write(data_len);

  serial_.write(kYawId);
  checksum = static_cast<uint8_t>(checksum + kYawId);
  for (uint8_t i = 0; i < data_len; ++i) {
    serial_.write(id1_data[i]);
    checksum = static_cast<uint8_t>(checksum + id1_data[i]);
  }

  serial_.write(kPitchId);
  checksum = static_cast<uint8_t>(checksum + kPitchId);
  for (uint8_t i = 0; i < data_len; ++i) {
    serial_.write(id2_data[i]);
    checksum = static_cast<uint8_t>(checksum + id2_data[i]);
  }

  serial_.write(static_cast<uint8_t>(~checksum));
  serial_.flush();
}

void CoreS3NeckDriver::writeWordBE(uint8_t* out, uint16_t value) {
  out[0] = static_cast<uint8_t>((value >> 8) & 0xFF);
  out[1] = static_cast<uint8_t>(value & 0xFF);
}

}  // namespace device
}  // namespace deskbot

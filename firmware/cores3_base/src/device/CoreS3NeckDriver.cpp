#include "CoreS3NeckDriver.h"

#include <math.h>

namespace deskbot {
namespace device {
namespace {

constexpr uint32_t kServoBaud = 1000000;
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
constexpr int16_t kMinRawChange = 2;

}  // namespace

bool CoreS3NeckDriver::begin(uint32_t now_ms) {
  serial_.begin(kServoBaud, SERIAL_8N1, kServoRxPin, kServoTxPin);
  delay(5);

  available_ = true;
  last_update_ms_ = now_ms;
  setTorque(true);
  writePositions(kYawZeroRaw, kPitchZeroRaw);
  last_yaw_raw_ = kYawZeroRaw;
  last_pitch_raw_ = kPitchZeroRaw;
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

  writePositions(yaw_raw, pitch_raw);
  last_yaw_raw_ = yaw_raw;
  last_pitch_raw_ = pitch_raw;
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

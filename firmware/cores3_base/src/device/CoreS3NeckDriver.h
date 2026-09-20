#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>

namespace deskbot {
namespace device {

struct NeckMotionCommand {
  uint32_t sequence = 0;
  uint32_t command_ms = 0;
  float previous_yaw_deg = 0.0f;
  float target_yaw_deg = 0.0f;
  float previous_pitch_deg = 0.0f;
  float target_pitch_deg = 0.0f;

  float yawDeltaDeg() const { return target_yaw_deg - previous_yaw_deg; }
  float pitchDeltaDeg() const { return target_pitch_deg - previous_pitch_deg; }
};

// Zero-base driver for the two serial-bus servos in the M5Stack Stack-chan body.
// Hardware facts are adapted from the repository's official Stack-chan donor:
// UART1 1Mbps, TX=6/RX=7, yaw ID=1, pitch ID=2.
// No donor class hierarchy is imported.
class CoreS3NeckDriver {
 public:
  bool begin(uint32_t now_ms);
  void tick(uint32_t now_ms, float yaw_norm, float pitch_norm);

  bool available() const { return available_; }
  bool servoPowerReady() const { return servo_power_ready_; }
  uint8_t servoPowerVersion() const { return servo_power_version_; }
  bool motionRecently(uint32_t now_ms) const;
  const NeckMotionCommand& lastMotionCommand() const { return last_motion_command_; }

 private:
  static float clampSigned(float value);
  static int16_t clampRaw(int16_t value, int16_t low, int16_t high);
  static int16_t normToRaw(float normalized,
                           int16_t zero_raw,
                           float max_degrees,
                           int16_t low_raw,
                           int16_t high_raw);

  bool enableServoPower();
  bool updatePy32Bit(uint8_t reg, uint8_t mask, bool enabled);
  void setTorque(bool enabled);
  void writePositions(int16_t yaw_raw, int16_t pitch_raw);
  void syncWrite(uint8_t address,
                 uint8_t data_len,
                 const uint8_t* id1_data,
                 const uint8_t* id2_data);
  void writeWordBE(uint8_t* out, uint16_t value);

  HardwareSerial serial_{1};
  bool available_ = false;
  bool servo_power_ready_ = false;
  uint8_t servo_power_version_ = 0;
  bool torque_enabled_ = false;
  uint32_t last_update_ms_ = 0;
  uint32_t last_motion_command_ms_ = 0;
  int16_t last_yaw_raw_ = -32768;
  int16_t last_pitch_raw_ = -32768;
  NeckMotionCommand last_motion_command_{};
};

}  // namespace device
}  // namespace deskbot

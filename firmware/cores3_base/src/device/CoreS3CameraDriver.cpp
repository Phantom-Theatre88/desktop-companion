#include "CoreS3CameraDriver.h"

#include <M5Unified.h>
#include <esp_camera.h>

namespace deskbot {
namespace device {

namespace {

camera_config_t makeCoreS3CameraConfig() {
  camera_config_t config{};
  config.pin_pwdn = -1;
  config.pin_reset = -1;
  config.pin_xclk = -1;
  config.pin_sscb_sda = 12;
  config.pin_sscb_scl = 11;
  config.pin_d7 = 47;
  config.pin_d6 = 48;
  config.pin_d5 = 16;
  config.pin_d4 = 15;
  config.pin_d3 = 42;
  config.pin_d2 = 41;
  config.pin_d1 = 40;
  config.pin_d0 = 39;
  config.pin_vsync = 46;
  config.pin_href = 38;
  config.pin_pclk = 45;
  config.xclk_freq_hz = 20000000;
  config.ledc_timer = LEDC_TIMER_0;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.pixel_format = PIXFORMAT_RGB565;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 0;
  config.fb_count = 2;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.sccb_i2c_port = -1;
  return config;
}

}  // namespace

CameraProbeResult CoreS3CameraDriver::probeOnce() {
  CameraProbeResult result;

  // CoreS3's GC0308 SCCB shares GPIO11/12 with M5Unified's internal I2C.
  // Follow M5Stack's official camera example and release the internal bus
  // before esp_camera_init(). This first implementation intentionally performs
  // a one-shot probe only, then deinitializes the camera and restores the
  // existing M5Unified I2C bus so Touch/IMU remain the standing DeskRobo senses.
  M5.In_I2C.release();

  camera_config_t config = makeCoreS3CameraConfig();
  const esp_err_t init_result = esp_camera_init(&config);
  if (init_result != ESP_OK) {
    result.internal_i2c_restored = M5.In_I2C.begin();
    return result;
  }

  result.initialized = true;

  if (sensor_t* sensor = esp_camera_sensor_get()) {
    sensor->set_framesize(sensor, FRAMESIZE_QVGA);
  }

  camera_fb_t* frame = esp_camera_fb_get();
  if (frame != nullptr) {
    result.captured = true;
    result.width = frame->width;
    result.height = frame->height;
    result.bytes = frame->len;
    result.average_luma = estimateLumaRgb565(frame->buf, frame->len);
    esp_camera_fb_return(frame);
  }

  esp_camera_deinit();
  result.internal_i2c_restored = M5.In_I2C.begin();
  return result;
}

uint8_t CoreS3CameraDriver::estimateLumaRgb565(const uint8_t* data, size_t bytes) {
  if (data == nullptr || bytes < 2) {
    return 0;
  }

  uint32_t sum = 0;
  uint32_t count = 0;

  // A sparse sample is enough for the first camera diagnostic and avoids doing
  // image-processing work inside the Device Driver layer.
  constexpr size_t kStrideBytes = 512;
  for (size_t i = 0; i + 1 < bytes; i += kStrideBytes) {
    const uint16_t pixel = static_cast<uint16_t>(data[i]) |
                           (static_cast<uint16_t>(data[i + 1]) << 8);
    const uint8_t r5 = (pixel >> 11) & 0x1F;
    const uint8_t g6 = (pixel >> 5) & 0x3F;
    const uint8_t b5 = pixel & 0x1F;

    const uint16_t r = (r5 * 255u) / 31u;
    const uint16_t g = (g6 * 255u) / 63u;
    const uint16_t b = (b5 * 255u) / 31u;
    const uint16_t y = static_cast<uint16_t>((r * 30u + g * 59u + b * 11u) / 100u);

    sum += y;
    ++count;
  }

  return count == 0 ? 0 : static_cast<uint8_t>(sum / count);
}

}  // namespace device
}  // namespace deskbot

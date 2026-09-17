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

CameraCaptureResult CoreS3CameraDriver::captureOnce(
    uint32_t now_ms,
    CameraFrameConsumer* consumer) {
  CameraCaptureResult result;
  result.timestamp_ms = now_ms;

  // CoreS3's GC0308 SCCB shares GPIO11/12 with M5Unified's internal I2C.
  // For now we intentionally keep each capture transaction self-contained:
  // release internal I2C -> init/capture -> deinit camera -> restore internal I2C.
  // This is slower than a persistent camera session, but it keeps Touch/IMU
  // ownership explicit while we prove that repeated vision does not break the
  // already-working standalone DeskRobo senses.
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

    if (consumer != nullptr) {
      CameraFrameView view;
      view.data = frame->buf;
      view.bytes = frame->len;
      view.width = frame->width;
      view.height = frame->height;
      view.timestamp_ms = now_ms;
      consumer->onCameraFrame(view);
    }

    esp_camera_fb_return(frame);
  }

  esp_camera_deinit();
  result.internal_i2c_restored = M5.In_I2C.begin();
  return result;
}

}  // namespace device
}  // namespace deskbot

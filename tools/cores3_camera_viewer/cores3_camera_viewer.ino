#include <M5Unified.h>
#include <WiFi.h>
#include <esp_camera.h>
#include <esp_http_server.h>
#include <img_converters.h>

#if __has_include("wifi_secrets.h")
#include "wifi_secrets.h"
#define DESKROBO_HAS_WIFI_SECRETS 1
#else
#define DESKROBO_HAS_WIFI_SECRETS 0
#endif

// Diagnostic-only CoreS3 camera viewer.
// This sketch is intentionally separate from the production DeskRobo runtime.

namespace {

constexpr char kApSsid[] = "DeskRobo-Camera";
constexpr char kApPassword[] = "deskrobo";  // Fallback only.
constexpr uint32_t kStaConnectTimeoutMs = 15000;
constexpr uint16_t kWidth = 320;
constexpr uint16_t kHeight = 240;

constexpr size_t kMaskColumns = 16;
constexpr size_t kMaskRows = 12;
constexpr size_t kMaskCells = kMaskColumns * kMaskRows;
constexpr uint32_t kMaskIntervalMs = 2000;
constexpr int kCellChangeThreshold = 20;

uint8_t previous_grid[kMaskCells]{};
bool latest_mask[kMaskCells]{};
bool have_previous_grid = false;
uint8_t previous_mean = 0;
uint32_t last_mask_sample_ms = 0;

httpd_handle_t server = nullptr;


uint8_t rgb565Luma(const uint8_t* data, size_t pixel_index) {
  const size_t i = pixel_index * 2;
  const uint16_t pixel =
      (static_cast<uint16_t>(data[i]) << 8) |
      static_cast<uint16_t>(data[i + 1]);
  const uint16_t r = ((pixel >> 11) & 31u) * 255u / 31u;
  const uint16_t g = ((pixel >> 5) & 63u) * 255u / 63u;
  const uint16_t b = (pixel & 31u) * 255u / 31u;
  return static_cast<uint8_t>((r * 30u + g * 59u + b * 11u) / 100u);
}

void sampleGrid(const camera_fb_t* frame, uint8_t* grid, uint8_t& mean) {
  uint32_t sum = 0;
  for (size_t row = 0; row < kMaskRows; ++row) {
    for (size_t col = 0; col < kMaskColumns; ++col) {
      uint16_t cell = 0;
      for (size_t dy = 1; dy <= 3; dy += 2) {
        for (size_t dx = 1; dx <= 3; dx += 2) {
          const size_t x =
              ((col * 4 + dx) * frame->width) / (kMaskColumns * 4);
          const size_t y =
              ((row * 4 + dy) * frame->height) / (kMaskRows * 4);
          cell += rgb565Luma(frame->buf, y * frame->width + x);
        }
      }
      const uint8_t value = static_cast<uint8_t>(cell / 4);
      grid[row * kMaskColumns + col] = value;
      sum += value;
    }
  }
  mean = static_cast<uint8_t>(sum / kMaskCells);
}

void updateMotionMask(camera_fb_t* frame, uint32_t now_ms) {
  if (have_previous_grid &&
      now_ms - last_mask_sample_ms < kMaskIntervalMs) {
    return;
  }

  uint8_t grid[kMaskCells];
  uint8_t mean = 0;
  sampleGrid(frame, grid, mean);

  for (size_t i = 0; i < kMaskCells; ++i) {
    latest_mask[i] = false;
  }

  if (have_previous_grid) {
    const int luma_shift = static_cast<int>(mean) -
                           static_cast<int>(previous_mean);
    size_t changed = 0;

    for (size_t i = 0; i < kMaskCells; ++i) {
      const int residual =
          static_cast<int>(grid[i]) -
          static_cast<int>(previous_grid[i]) -
          luma_shift;
      const int magnitude = residual < 0 ? -residual : residual;
      if (magnitude >= kCellChangeThreshold) {
        latest_mask[i] = true;
        ++changed;
      }
    }

    Serial.printf("[VIEWER][MASK] changed=%u/%u motion=%.3f luma_shift=%d\n",
                  static_cast<unsigned>(changed),
                  static_cast<unsigned>(kMaskCells),
                  static_cast<float>(changed) /
                      static_cast<float>(kMaskCells),
                  luma_shift);
  } else {
    Serial.println("[VIEWER][MASK] baseline captured");
  }

  for (size_t i = 0; i < kMaskCells; ++i) {
    previous_grid[i] = grid[i];
  }
  previous_mean = mean;
  have_previous_grid = true;
  last_mask_sample_ms = now_ms;
}

void tintPixelRed(uint8_t* data, size_t pixel_index) {
  const size_t i = pixel_index * 2;
  const uint16_t pixel =
      (static_cast<uint16_t>(data[i]) << 8) |
      static_cast<uint16_t>(data[i + 1]);

  const uint16_t r = (pixel >> 11) & 31u;
  const uint16_t g = (pixel >> 5) & 63u;
  const uint16_t b = pixel & 31u;

  // Roughly 2/3 red overlay while preserving enough of the live image.
  const uint16_t out_r = static_cast<uint16_t>((r + 31u * 2u) / 3u);
  const uint16_t out_g = static_cast<uint16_t>(g / 3u);
  const uint16_t out_b = static_cast<uint16_t>(b / 3u);
  const uint16_t out =
      static_cast<uint16_t>((out_r << 11) | (out_g << 5) | out_b);

  data[i] = static_cast<uint8_t>(out >> 8);
  data[i + 1] = static_cast<uint8_t>(out & 0xff);
}

void drawMotionMask(camera_fb_t* frame) {
  for (size_t row = 0; row < kMaskRows; ++row) {
    for (size_t col = 0; col < kMaskColumns; ++col) {
      if (!latest_mask[row * kMaskColumns + col]) {
        continue;
      }

      const size_t x0 = (col * frame->width) / kMaskColumns;
      const size_t x1 = ((col + 1) * frame->width) / kMaskColumns;
      const size_t y0 = (row * frame->height) / kMaskRows;
      const size_t y1 = ((row + 1) * frame->height) / kMaskRows;

      // Checkerboard tint keeps the underlying camera image readable.
      for (size_t y = y0; y < y1; y += 2) {
        for (size_t x = x0; x < x1; x += 2) {
          tintPixelRed(frame->buf, y * frame->width + x);
        }
      }
    }
  }
}

camera_config_t makeCameraConfig() {
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

  // GC0308 output stays RGB565, matching the production driver.
  // Each frame is converted to JPEG for browser streaming.
  config.pixel_format = PIXFORMAT_RGB565;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 0;
  config.fb_count = 2;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.sccb_i2c_port = -1;
  return config;
}

esp_err_t indexHandler(httpd_req_t* req) {
  static const char page[] =
      "<!doctype html><html><head>"
      "<meta name='viewport' content='width=device-width,initial-scale=1'>"
      "<title>DeskRobo Camera Viewer</title>"
      "<style>"
      "body{margin:0;background:#111;color:#eee;font-family:-apple-system,BlinkMacSystemFont,sans-serif;"
      "display:flex;min-height:100vh;align-items:center;justify-content:center}"
      "main{width:min(94vw,960px);text-align:center}"
      "h1{font-size:20px;font-weight:600;margin:0 0 12px}"
      "p{color:#aaa;font-size:13px;margin:8px 0 14px}"
      ".frame{position:relative;display:inline-block;max-width:100%;background:#000}"
      "img{display:block;width:min(92vw,800px);height:auto;image-rendering:auto}"
      ".grid{pointer-events:none;position:absolute;inset:0;"
      "background-image:linear-gradient(to right,rgba(255,255,255,.16) 1px,transparent 1px),"
      "linear-gradient(to bottom,rgba(255,255,255,.16) 1px,transparent 1px);"
      "background-size:6.25% 8.333333%}"
      "</style></head><body><main>"
      "<h1>CoreS3 Camera — Live</h1>"
      "<p>Grid = Vision 16×12. Red cells = motion mask sampled about every 2 seconds.</p>"
      "<div class='frame'><img src='/stream' alt='CoreS3 live camera'><div class='grid'></div></div>"
      "</main></body></html>";

  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_send(req, page, HTTPD_RESP_USE_STRLEN);
}

esp_err_t streamHandler(httpd_req_t* req) {
  static const char kStreamType[] =
      "multipart/x-mixed-replace;boundary=frame";
  static const char kBoundary[] = "\r\n--frame\r\n";
  static const char kPartHeader[] =
      "Content-Type: image/jpeg\r\n"
      "Content-Length: %u\r\n\r\n";

  esp_err_t result = httpd_resp_set_type(req, kStreamType);
  if (result != ESP_OK) {
    return result;
  }
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");

  char header[96];

  while (true) {
    camera_fb_t* frame = esp_camera_fb_get();
    if (frame == nullptr) {
      Serial.println("[VIEWER][ERROR] esp_camera_fb_get failed");
      delay(30);
      continue;
    }

    updateMotionMask(frame, millis());
    drawMotionMask(frame);

    uint8_t* jpg = nullptr;
    size_t jpg_len = 0;
    const bool converted =
        frame2jpg(frame, 70, &jpg, &jpg_len);

    esp_camera_fb_return(frame);

    if (!converted || jpg == nullptr || jpg_len == 0) {
      if (jpg != nullptr) {
        free(jpg);
      }
      Serial.println("[VIEWER][ERROR] RGB565 -> JPEG conversion failed");
      delay(30);
      continue;
    }

    result = httpd_resp_send_chunk(req, kBoundary, strlen(kBoundary));
    if (result == ESP_OK) {
      const int header_len =
          snprintf(header, sizeof(header), kPartHeader,
                   static_cast<unsigned>(jpg_len));
      result = httpd_resp_send_chunk(req, header, header_len);
    }
    if (result == ESP_OK) {
      result = httpd_resp_send_chunk(
          req, reinterpret_cast<const char*>(jpg), jpg_len);
    }

    free(jpg);

    if (result != ESP_OK) {
      Serial.println("[VIEWER] Browser stream disconnected");
      break;
    }

    // Keep diagnostic streaming responsive without starving Wi-Fi.
    delay(10);
  }

  return result;
}

bool startCamera() {
  // CoreS3 GC0308 SCCB uses the same GPIO11/12 pair as M5Unified internal I2C.
  // This diagnostic sketch does not use Touch/IMU, so release the internal bus
  // and leave the camera initialized continuously.
  M5.In_I2C.release();

  camera_config_t config = makeCameraConfig();
  const esp_err_t init_result = esp_camera_init(&config);
  if (init_result != ESP_OK) {
    Serial.printf("[VIEWER][ERROR] Camera init failed: 0x%x\n",
                  static_cast<unsigned>(init_result));
    return false;
  }

  sensor_t* sensor = esp_camera_sensor_get();
  if (sensor != nullptr) {
    sensor->set_framesize(sensor, FRAMESIZE_QVGA);
  }

  return true;
}

bool startServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  config.max_uri_handlers = 4;
  config.stack_size = 8192;

  if (httpd_start(&server, &config) != ESP_OK) {
    return false;
  }

  httpd_uri_t index_uri{};
  index_uri.uri = "/";
  index_uri.method = HTTP_GET;
  index_uri.handler = indexHandler;

  httpd_uri_t stream_uri{};
  stream_uri.uri = "/stream";
  stream_uri.method = HTTP_GET;
  stream_uri.handler = streamHandler;

  if (httpd_register_uri_handler(server, &index_uri) != ESP_OK) {
    return false;
  }
  if (httpd_register_uri_handler(server, &stream_uri) != ESP_OK) {
    return false;
  }

  return true;
}

void showStatus(const char* line1, const char* line2) {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextSize(2);
  M5.Display.drawString(line1,
                        M5.Display.width() / 2,
                        M5.Display.height() / 2 - 18);
  M5.Display.setTextSize(1);
  M5.Display.drawString(line2,
                        M5.Display.width() / 2,
                        M5.Display.height() / 2 + 18);
}

}  // namespace

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  Serial.begin(115200);
  delay(300);

  showStatus("CAMERA VIEWER", "starting...");

  if (!startCamera()) {
    showStatus("CAMERA ERROR", "see Serial");
    return;
  }

  bool using_fallback_ap = false;
  IPAddress viewer_ip;

#if DESKROBO_HAS_WIFI_SECRETS
  Serial.printf("[VIEWER] Connecting to Wi-Fi SSID: %s\n", WIFI_SSID);
  showStatus("CAMERA VIEWER", "joining Wi-Fi...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  const uint32_t connect_started_ms = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - connect_started_ms < kStaConnectTimeoutMs) {
    delay(250);
  }

  if (WiFi.status() == WL_CONNECTED) {
    viewer_ip = WiFi.localIP();
    Serial.println("[VIEWER] LAN Wi-Fi connected");
  } else {
    Serial.println("[VIEWER][WARN] LAN Wi-Fi failed; falling back to DeskRobo-Camera");
    WiFi.disconnect(true);
    delay(100);
    using_fallback_ap = true;
  }
#else
  Serial.println("[VIEWER][WARN] wifi_secrets.h not found; using fallback SoftAP");
  using_fallback_ap = true;
#endif

  if (using_fallback_ap) {
    WiFi.mode(WIFI_AP);
    if (!WiFi.softAP(kApSsid, kApPassword)) {
      Serial.println("[VIEWER][ERROR] SoftAP start failed");
      showStatus("WIFI ERROR", "see Serial");
      return;
    }
    viewer_ip = WiFi.softAPIP();
  }

  if (!startServer()) {
    Serial.println("[VIEWER][ERROR] HTTP server start failed");
    showStatus("HTTP ERROR", "see Serial");
    return;
  }

  Serial.println("[VIEWER] READY");
  if (using_fallback_ap) {
    Serial.printf("[VIEWER] Fallback Wi-Fi SSID: %s\n", kApSsid);
    Serial.printf("[VIEWER] Fallback Wi-Fi password: %s\n", kApPassword);
  } else {
    Serial.println("[VIEWER] Mode: LOCAL LAN ONLY");
  }
  Serial.printf("[VIEWER] Open: http://%s/\n", viewer_ip.toString().c_str());

  const String ip_text = viewer_ip.toString();
  showStatus("CAMERA VIEWER", ip_text.c_str());
}

void loop() {
  // Web server owns streaming. Keep M5 alive but do not touch internal I2C.
  delay(1000);
}

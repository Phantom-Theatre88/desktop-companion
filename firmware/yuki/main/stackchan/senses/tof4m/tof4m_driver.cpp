#include "stackchan/senses/tof4m/tof4m_driver.h"

#include <array>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace yuki::senses::tof4m {
namespace {

constexpr char kTag[] = "YukiToF4M";
constexpr uint8_t kAddress = 0x29;
constexpr gpio_num_t kSda = GPIO_NUM_2;
constexpr gpio_num_t kScl = GPIO_NUM_1;
constexpr uint32_t kI2cHz = 400000;

constexpr uint16_t kRegFirmwareStatus = 0x00E5;
constexpr uint16_t kRegModelId = 0x010F;
constexpr uint16_t kRegGpioMuxCtrl = 0x0030;
constexpr uint16_t kRegGpioStatus = 0x0031;
constexpr uint16_t kRegModeStart = 0x0087;
constexpr uint16_t kRegInterruptClear = 0x0086;
constexpr uint16_t kRegDistance = 0x0096;

// VL53L1X ULD default configuration bytes for registers 0x002D..0x0087.
// Source lineage: STMicroelectronics VL53L1X Ultra Lite Driver (BSD-3-Clause option).
constexpr std::array<uint8_t, 91> kDefaultConfig = {
    0x00,0x00,0x00,0x01,0x02,0x00,0x02,0x08,0x00,0x08,0x10,0x01,0x01,0x00,0x00,0x00,
    0x00,0xFF,0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x20,0x0B,0x00,0x00,0x02,0x0A,0x21,
    0x00,0x00,0x05,0x00,0x00,0x00,0x00,0xC8,0x00,0x00,0x38,0xFF,0x01,0x00,0x08,0x00,
    0x00,0x01,0xCC,0x0F,0x01,0xF1,0x0D,0x01,0x68,0x00,0x80,0x08,0xB8,0x00,0x00,0x00,
    0x00,0x0F,0x89,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x0F,0x0D,0x0E,
    0x0E,0x00,0x00,0x02,0xC7,0xFF,0x9B,0x00,0x00,0x00,0x01,0x00,0x00
};

}  // namespace

ToF4MDriver::~ToF4MDriver() {
    if (dev_ != nullptr) {
        i2c_master_bus_rm_device(dev_);
    }
    if (bus_ != nullptr) {
        i2c_del_master_bus(bus_);
    }
}

esp_err_t ToF4MDriver::write8(uint16_t reg, uint8_t value) {
    uint8_t buffer[3] = {static_cast<uint8_t>(reg >> 8), static_cast<uint8_t>(reg & 0xFF), value};
    return i2c_master_transmit(dev_, buffer, sizeof(buffer), pdMS_TO_TICKS(100));
}

esp_err_t ToF4MDriver::write_block(uint16_t reg, const uint8_t* data, size_t size) {
    if (size > 96) {
        return ESP_ERR_INVALID_SIZE;
    }
    uint8_t buffer[98] = {};
    buffer[0] = static_cast<uint8_t>(reg >> 8);
    buffer[1] = static_cast<uint8_t>(reg & 0xFF);
    for (size_t i = 0; i < size; ++i) {
        buffer[i + 2] = data[i];
    }
    return i2c_master_transmit(dev_, buffer, size + 2, pdMS_TO_TICKS(200));
}

esp_err_t ToF4MDriver::read8(uint16_t reg, uint8_t& value) {
    uint8_t addr[2] = {static_cast<uint8_t>(reg >> 8), static_cast<uint8_t>(reg & 0xFF)};
    return i2c_master_transmit_receive(dev_, addr, sizeof(addr), &value, 1, pdMS_TO_TICKS(100));
}

esp_err_t ToF4MDriver::read16(uint16_t reg, uint16_t& value) {
    uint8_t addr[2] = {static_cast<uint8_t>(reg >> 8), static_cast<uint8_t>(reg & 0xFF)};
    uint8_t data[2] = {};
    const esp_err_t err = i2c_master_transmit_receive(dev_, addr, sizeof(addr), data, sizeof(data), pdMS_TO_TICKS(100));
    if (err == ESP_OK) {
        value = (static_cast<uint16_t>(data[0]) << 8) | data[1];
    }
    return err;
}

esp_err_t ToF4MDriver::wait_boot(uint32_t timeout_ms) {
    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);
    while (xTaskGetTickCount() < deadline) {
        uint8_t state = 0;
        const esp_err_t err = read8(kRegFirmwareStatus, state);
        if (err == ESP_OK && state != 0) {
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return ESP_ERR_TIMEOUT;
}

esp_err_t ToF4MDriver::init() {
    if (initialized_) {
        return ESP_OK;
    }

    i2c_master_bus_config_t bus_cfg = {};
    bus_cfg.i2c_port = I2C_NUM_0;
    bus_cfg.sda_io_num = kSda;
    bus_cfg.scl_io_num = kScl;
    bus_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_cfg.glitch_ignore_cnt = 7;
    bus_cfg.flags.enable_internal_pullup = true;

    esp_err_t err = i2c_new_master_bus(&bus_cfg, &bus_);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "Unable to create Grove I2C bus: %s", esp_err_to_name(err));
        return err;
    }

    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address = kAddress;
    dev_cfg.scl_speed_hz = kI2cHz;
    err = i2c_master_bus_add_device(bus_, &dev_cfg, &dev_);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "Unable to add U172 device: %s", esp_err_to_name(err));
        return err;
    }

    err = i2c_master_probe(bus_, kAddress, pdMS_TO_TICKS(200));
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "U172 not detected at 0x29: %s", esp_err_to_name(err));
        return err;
    }

    err = wait_boot(1000);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "VL53L1X boot timeout");
        return err;
    }

    uint16_t model_id = 0;
    err = read16(kRegModelId, model_id);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "Unable to read VL53L1X model id: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(kTag, "U172 detected, VL53L1X model id=0x%04X", model_id);

    err = write_block(0x002D, kDefaultConfig.data(), kDefaultConfig.size());
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "VL53L1X default configuration failed: %s", esp_err_to_name(err));
        return err;
    }

    err = write8(kRegModeStart, 0x40);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "VL53L1X start ranging failed: %s", esp_err_to_name(err));
        return err;
    }

    initialized_ = true;
    ESP_LOGI(kTag, "U172 ranging started on Grove I2C0 SDA=2 SCL=1");
    return ESP_OK;
}

esp_err_t ToF4MDriver::read_distance_mm(uint16_t& distance_mm, bool& data_ready) {
    data_ready = false;
    if (!initialized_) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t mux = 0;
    uint8_t status = 0;
    esp_err_t err = read8(kRegGpioMuxCtrl, mux);
    if (err != ESP_OK) return err;
    err = read8(kRegGpioStatus, status);
    if (err != ESP_OK) return err;

    const uint8_t interrupt_polarity = static_cast<uint8_t>(!((mux & 0x10) >> 4));
    data_ready = (status & 0x01) == interrupt_polarity;
    if (!data_ready) {
        return ESP_OK;
    }

    err = read16(kRegDistance, distance_mm);
    if (err != ESP_OK) return err;

    return write8(kRegInterruptClear, 0x01);
}

}  // namespace yuki::senses::tof4m

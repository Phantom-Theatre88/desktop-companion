#pragma once

#include <cstdint>
#include <driver/i2c_master.h>
#include <esp_err.h>

namespace yuki::senses::tof4m {

class ToF4MDriver {
public:
    ~ToF4MDriver();

    esp_err_t init();
    esp_err_t read_distance_mm(uint16_t& distance_mm, bool& data_ready);
    bool initialized() const { return initialized_; }

private:
    esp_err_t write8(uint16_t reg, uint8_t value);
    esp_err_t write_block(uint16_t reg, const uint8_t* data, size_t size);
    esp_err_t read8(uint16_t reg, uint8_t& value);
    esp_err_t read16(uint16_t reg, uint16_t& value);
    esp_err_t wait_boot(uint32_t timeout_ms);

    i2c_master_bus_handle_t bus_ = nullptr;
    i2c_master_dev_handle_t dev_ = nullptr;
    bool initialized_ = false;
};

}  // namespace yuki::senses::tof4m

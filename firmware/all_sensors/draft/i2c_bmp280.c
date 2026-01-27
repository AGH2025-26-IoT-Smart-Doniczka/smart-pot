#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_bmp280.h"

#define I2C_BMP280_ADDRESS 0x76
#define I2C_BMP280_CLOCK_SPEED 400000

static const char* TAG = "BMP280";

i2c_master_dev_handle_t bmp820_init_device(i2c_master_bus_handle_t bus_handle) {
    i2c_master_dev_handle_t dev_handle; 

    ESP_LOGI(TAG, "Initializing BMP280 device config...");
    
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_BMP280_ADDRESS,       
        .scl_speed_hz = I2C_BMP280_CLOCK_SPEED,     
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle));
    
    ESP_LOGI(TAG, "Device added, returning handle.");
    return dev_handle; 
}

static int8_t bme280_i2c_read_cb(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    if ((reg_data == NULL) || (intf_ptr == NULL)) {
        return BME280_E_NULL_PTR;
    }

    i2c_master_dev_handle_t handle = (i2c_master_dev_handle_t)intf_ptr;
    esp_err_t err = i2c_master_transmit_receive(handle,
                                                &reg_addr,
                                                1,
                                                reg_data,
                                                len,
                                                pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C read failed (%s)", esp_err_to_name(err));
        return BME280_E_COMM_FAIL;
    }

    return BME280_OK;
}

static int8_t bme280_i2c_write_cb(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    if (intf_ptr == NULL) {
        return BME280_E_NULL_PTR;
    }

    i2c_master_dev_handle_t handle = (i2c_master_dev_handle_t)intf_ptr;
    uint8_t write_buf[1 + len];
    write_buf[0] = reg_addr;
    if ((reg_data != NULL) && (len > 0)) {
        memcpy(&write_buf[1], reg_data, len);
    }

    esp_err_t err = i2c_master_transmit(handle,
                                        write_buf,
                                        len + 1,
                                        pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C write failed (%s)", esp_err_to_name(err));
        return BME280_E_COMM_FAIL;
    }

    return BME280_OK;
}

static void bme280_delay_us_cb(uint32_t period, void *intf_ptr)
{
    (void)intf_ptr;
    esp_rom_delay_us(period);
}

esp_err_t bme280_setup_device(i2c_master_dev_handle_t dev_handle, struct bme280_dev *dev) {
    dev->intf = BME280_I2C_INTF;
    dev->read = bme280_i2c_read_cb;
    dev->write = bme280_i2c_write_cb;
    dev->delay_us = bme280_delay_us_cb;
    dev->intf_ptr = dev_handle;

    if (BME280_OK != bme280_init(dev)) {
        ESP_LOGW(TAG, "bme280_init failed");
        return ESP_FAIL;
    }

    struct bme280_settings settings = {
        .osr_p = BME280_OVERSAMPLING_8X,
        .osr_t = BME280_OVERSAMPLING_2X,
        .osr_h = BME280_OVERSAMPLING_1X,
        .filter = BME280_FILTER_COEFF_16,
        .standby_time = BME280_STANDBY_TIME_62_5_MS,
    };

    uint8_t settings_sel = BME280_SEL_OSR_PRESS |
                           BME280_SEL_OSR_TEMP |
                           BME280_SEL_OSR_HUM |
                           BME280_SEL_FILTER |
                           BME280_SEL_STANDBY;

    if (BME280_OK != bme280_set_sensor_settings(settings_sel, &settings, dev)) {
        ESP_LOGE(TAG, "bme280_set_sensor_settings failed");
        return ESP_FAIL;
    }

    if (BME280_OK != bme280_set_sensor_mode(BME280_POWERMODE_NORMAL, dev)) {
        ESP_LOGE(TAG, "bme280_set_sensor_mode failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}
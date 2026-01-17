#ifndef I2C_BMP280_H
#define I2C_BMP280_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "bme280.h"

#ifdef __cplusplus
extern "C" {
#endif

#define I2C_MASTER_TIMEOUT_MS 1000

i2c_master_dev_handle_t bmp820_init_device(i2c_master_bus_handle_t bus_handle);
esp_err_t bme280_setup_device(i2c_master_dev_handle_t dev_handle, struct bme280_dev *dev);

#ifdef __cplusplus
}
#endif

#endif // I2C_BMP280_H
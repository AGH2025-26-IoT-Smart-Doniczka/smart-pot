#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "driver/i2c_master.h"
#include "app_context.h"
#include "bme280.h"
#include "bme280_defs.h"
#include "bme280_task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* =========================================================================
   SECTION: Constants
   ========================================================================= */
#define BME280_I2C_ADDR_PRIMARY    0x76
#define BME280_I2C_SPEED_HZ        100000
#define BME280_TIMEOUT_MS          2000
#define BME280_TASK_STACK          4096
#define BME280_READ_ATTEMPTS        3
#define BME280_READ_DELAY_MS       50
#define BME280_DEBUG_INTERVAL_MS  500

/* =========================================================================
   SECTION: Static Data
   ========================================================================= */
static const char *TAG = "BME_TASK";

/* =========================================================================
   SECTION: I2C Callbacks
   ========================================================================= */
static int8_t bme280_i2c_read_cb(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    if ((reg_data == NULL) || (intf_ptr == NULL)) {
        return BME280_E_NULL_PTR;
    }

    i2c_master_dev_handle_t handle = *((i2c_master_dev_handle_t *)intf_ptr);
    esp_err_t err = i2c_master_transmit_receive(handle,
                                                &reg_addr,
                                                1,
                                                reg_data,
                                                len,
                                                BME280_TIMEOUT_MS);
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

    i2c_master_dev_handle_t handle = *((i2c_master_dev_handle_t *)intf_ptr);
    uint8_t write_buf[1 + len];
    write_buf[0] = reg_addr;
    if ((reg_data != NULL) && (len > 0U)) {
        memcpy(&write_buf[1], reg_data, len);
    }

    esp_err_t err = i2c_master_transmit(handle,
                                        write_buf,
                                        len + 1U,
                                        BME280_TIMEOUT_MS);
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

static void bme280_wait_measurement(struct bme280_dev *dev)
{
    if (dev == NULL) {
        return;
    }

    struct bme280_settings settings = {0};
    uint32_t delay_us = 0;
    if (BME280_OK == bme280_get_sensor_settings(&settings, dev)) {
        (void)bme280_cal_meas_delay(&delay_us, &settings);
    }

    if (delay_us < 10000U) {
        delay_us = 10000U;
    }

    dev->delay_us(delay_us + 5000U, dev->intf_ptr);
}


static void bme280_force_measurement(struct bme280_dev *dev)
{
    if (dev == NULL) {
        return;
    }

    (void)bme280_set_sensor_mode(BME280_POWERMODE_FORCED, dev);
    bme280_wait_measurement(dev);
}

/* =========================================================================
   SECTION: Helpers
   ========================================================================= */
static esp_err_t bme280_setup(struct bme280_dev *dev)
{
    if (BME280_OK != bme280_init(dev)) {
        ESP_LOGW(TAG, "bme280_init failed");
        return ESP_FAIL;
    }

    struct bme280_settings settings = {
        .osr_p = BME280_OVERSAMPLING_16X,
        .osr_t = BME280_OVERSAMPLING_2X,
        .osr_h = BME280_NO_OVERSAMPLING,
        .filter = BME280_FILTER_COEFF_16,
        .standby_time = BME280_STANDBY_TIME_62_5_MS,
    };

    uint8_t settings_sel = BME280_SEL_OSR_PRESS |
                           BME280_SEL_OSR_TEMP |
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

    bme280_wait_measurement(dev);

    return ESP_OK;
}

static void bme280_log_chip_id(struct bme280_dev *dev)
{
    if (dev == NULL) {
        return;
    }

    uint8_t chip_id = 0;
    if (BME280_OK == bme280_get_regs(BME280_REG_CHIP_ID, &chip_id, 1, dev)) {
        ESP_LOGI(TAG, "chip id=0x%02X", chip_id);
    }
}

static uint16_t temp_c_to_dk(float temp_c)
{
    float temp_dk_f = (temp_c + 273.15f) * 10.0f;
    if (temp_dk_f < 0.0f) {
        return 0U;
    }
    if (temp_dk_f > 65535.0f) {
        return 65535U;
    }
    return (uint16_t)(temp_dk_f + 0.5f);
}

static void bme280_update_context(const sensor_task_context_t *shared, float temp_c, float pressure_pa)
{
    if (shared == NULL || shared->data == NULL) {
        return;
    }

    shared->data->temperature = temp_c_to_dk(temp_c);
    shared->data->pressure = pressure_pa / 100.0f;
}

static void bme280_comp_to_float(const struct bme280_data *data, float *out_temp_c, float *out_pressure_pa)
{
    if (data == NULL || out_temp_c == NULL || out_pressure_pa == NULL) {
        return;
    }

#ifdef BME280_DOUBLE_ENABLE
    *out_temp_c = (float)data->temperature;
    *out_pressure_pa = (float)data->pressure;
#else
    *out_temp_c = (float)data->temperature / 100.0f;
    *out_pressure_pa = (float)data->pressure / 100.0f;
#endif
}

/* =========================================================================
   SECTION: Task
   ========================================================================= */
static esp_err_t bme280_read_impl(sensor_task_context_t *shared_ctx)
{
    if (shared_ctx == NULL || shared_ctx->bus == NULL) {
        ESP_LOGW(TAG, "shared context missing");
        return ESP_ERR_INVALID_ARG;
    }

    i2c_master_bus_handle_t bus = shared_ctx->bus;
    ESP_LOGI(TAG, "bus=%p", (void *)bus);

    i2c_master_dev_handle_t dev_handle = NULL;
    struct bme280_dev dev = {0};
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BME280_I2C_ADDR_PRIMARY,
        .scl_speed_hz = BME280_I2C_SPEED_HZ,
    };

    if (i2c_master_bus_add_device(bus, &dev_config, &dev_handle) != ESP_OK) {
        ESP_LOGW(TAG, "device add failed");
        goto cleanup;
    }

    ESP_LOGI(TAG, "device added addr=0x%02X", BME280_I2C_ADDR_PRIMARY);

    dev.intf = BME280_I2C_INTF;
    dev.read = bme280_i2c_read_cb;
    dev.write = bme280_i2c_write_cb;
    dev.delay_us = bme280_delay_us_cb;
    dev.intf_ptr = &dev_handle;

    if (bme280_setup(&dev) != ESP_OK) {
        ESP_LOGW(TAG, "bme280 setup failed");
        goto cleanup;
    }

    bme280_log_chip_id(&dev);

    struct bme280_data comp_data = {0};
    bool read_ok = false;
    for (uint32_t i = 0; i < BME280_READ_ATTEMPTS; ++i) {
        bme280_force_measurement(&dev);
        if (BME280_OK == bme280_get_sensor_data(BME280_PRESS | BME280_TEMP, &comp_data, &dev)) {
            if (!((comp_data.temperature == 0.0f) && (comp_data.pressure == 0.0f))) {
                read_ok = true;
                break;
            }
            ESP_LOGW(TAG, "bme280 data zero (attempt %u)", (unsigned)(i + 1U));
        } else {
            ESP_LOGW(TAG, "bme280_get_sensor_data failed (attempt %u)", (unsigned)(i + 1U));
        }

        vTaskDelay(pdMS_TO_TICKS(BME280_READ_DELAY_MS));
    }

    if (!read_ok) {
        ESP_LOGW(TAG, "bme280 data invalid after retries");
        goto cleanup;
    }

    float temp_c = 0.0f;
    float pressure_pa = 0.0f;
    bme280_comp_to_float(&comp_data, &temp_c, &pressure_pa);

    ESP_LOGI(TAG, "raw temp=%.2fC press=%.2fPa", temp_c, pressure_pa);

    bme280_update_context(shared_ctx, temp_c, pressure_pa);
    (void)bme280_set_sensor_mode(BME280_POWERMODE_SLEEP, &dev);

cleanup:
    if (dev_handle != NULL) {
        (void)i2c_master_bus_rm_device(dev_handle);
    }

    ESP_LOGI(TAG, "bme280 task done");
    return ESP_OK;
}

/* =========================================================================
   SECTION: Public API
   ========================================================================= */
esp_err_t bme280_read_once(sensor_task_context_t *shared_ctx)
{
    return bme280_read_impl(shared_ctx);
    
}

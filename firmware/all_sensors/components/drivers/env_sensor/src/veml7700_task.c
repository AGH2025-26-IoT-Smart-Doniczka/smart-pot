#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "app_context.h"
#include "veml7700.h"
#include "veml7700_task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* =========================================================================
   SECTION: Constants
   ========================================================================= */
#define VEML7700_I2C_ADDR       0x10
#define VEML7700_I2C_SPEED_HZ   100000
#define VEML7700_TIMEOUT_MS     500
#define VEML7700_TASK_STACK     4096
#define VEML7700_TASK_PRIO      5

#define LUX_LOW_THRESHOLD       300.0f
#define LUX_HIGH_THRESHOLD      1000.0f

/* =========================================================================
   SECTION: Static Data
   ========================================================================= */
static const char *TAG = "VEML_TASK";

/* =========================================================================
   SECTION: Helpers
   ========================================================================= */
static uint16_t lux_to_level(float lux)
{
    if (lux < LUX_LOW_THRESHOLD) {
        return 2U;
    }
    if (lux < LUX_HIGH_THRESHOLD) {
        return 1U;
    }
    return 0U;
}

static void veml7700_update_context(const sensor_task_context_t *shared, float lux)
{
    if (shared == NULL || shared->data == NULL) {
        return;
    }

    shared->data->lux_level = (uint16_t)lux_to_level(lux);
}

static uint32_t veml7700_get_integration_ms(uint16_t config)
{
    uint16_t it = config & VEML7700_ALS_IT_MASK;
    switch (it) {
        case VEML7700_ALS_IT_25MS:
            return 25;
        case VEML7700_ALS_IT_50MS:
            return 50;
        case VEML7700_ALS_IT_100MS:
            return 100;
        case VEML7700_ALS_IT_200MS:
            return 200;
        case VEML7700_ALS_IT_400MS:
            return 400;
        case VEML7700_ALS_IT_800MS:
            return 800;
        default:
            return 100;
    }
}

/* =========================================================================
   SECTION: Task
   ========================================================================= */
static esp_err_t veml7700_read_impl(sensor_task_context_t *shared_ctx)
{
    if (shared_ctx == NULL || shared_ctx->bus == NULL) {
        ESP_LOGW(TAG, "shared context missing");
        return ESP_ERR_INVALID_ARG;
    }

    i2c_master_bus_handle_t bus = shared_ctx->bus;
    ESP_LOGI(TAG, "bus=%p", (void *)bus);

    i2c_master_dev_handle_t dev_handle = NULL;
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = VEML7700_I2C_ADDR,
        .scl_speed_hz = VEML7700_I2C_SPEED_HZ,
    };

    if (i2c_master_bus_add_device(bus, &dev_cfg, &dev_handle) != ESP_OK) {
        ESP_LOGW(TAG, "device add failed");
        goto cleanup_lock;
    }
    ESP_LOGI(TAG, "device added addr=0x%02X", VEML7700_I2C_ADDR);

    uint16_t cfg = VEML7700_DEFAULT_CONFIG;
    if (veml7700_init(dev_handle, cfg) != ESP_OK) {
        ESP_LOGW(TAG, "veml7700 init failed");
        goto cleanup;
    }

    if (veml7700_read_config(dev_handle, &cfg) != ESP_OK) {
        ESP_LOGW(TAG, "veml7700 read config failed");
        cfg = VEML7700_DEFAULT_CONFIG;
    }

    uint32_t wait_ms = veml7700_get_integration_ms(cfg) + 10U;
    if (wait_ms < 150U) {
        wait_ms = 150U;
    }
    vTaskDelay(pdMS_TO_TICKS(wait_ms));

    float lux = 0.0f;
    if (veml7700_read_lux(dev_handle, &lux) != ESP_OK) {
        ESP_LOGW(TAG, "veml7700 read lux failed");
        goto cleanup;
    }

    ESP_LOGI(TAG, "cfg=0x%04X lux=%.2f", (unsigned)cfg, lux);
    veml7700_update_context(shared_ctx, lux);

cleanup:
    if (dev_handle != NULL) {
        (void)veml7700_shutdown(dev_handle);
        (void)i2c_master_bus_rm_device(dev_handle);
    }

cleanup_lock:
    ESP_LOGI(TAG, "veml7700 task done");
    return ESP_OK;
}

/* =========================================================================
   SECTION: Public API
   ========================================================================= */
esp_err_t veml7700_read_once(sensor_task_context_t *shared_ctx)
{
    return veml7700_read_impl(shared_ctx);
}

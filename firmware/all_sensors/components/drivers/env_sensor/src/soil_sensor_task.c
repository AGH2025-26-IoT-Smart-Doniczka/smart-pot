#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "board_pins.h"
#include "app_context.h"
#include "soil_sensor.h"
#include "soil_sensor_task.h"

/* =========================================================================
   SECTION: Constants
   ========================================================================= */
#define SOIL_TASK_STACK 4096
#define SOIL_TASK_PRIO  5

/* =========================================================================
   SECTION: Static Data
   ========================================================================= */
static const char *TAG = "SOIL_TASK";

/* =========================================================================
   SECTION: Helpers
   ========================================================================= */
static soil_sensor_handle_t ensure_soil_sensor(void)
{
    soil_sensor_handle_t handle = app_context_get_soil_sensor();
    if (handle != NULL) {
        return handle;
    }

    soil_sensor_config_t cfg = {
        .unit = BSP_ADC_UNIT,
        .channel = BSP_ADC_CHANNEL,
        .bitwidth = BSP_ADC_WIDTH,
        .atten = BSP_ADC_ATTEN,
        .power_gpio = GPIO_NUM_NC,
        .power_active_high = true,
        .sample_count = 0,
        .settle_ms = 0,
    };

    if (soil_sensor_create(&cfg, &handle) != ESP_OK) {
        return NULL;
    }

    (void)app_context_set_soil_sensor(handle);
    return handle;
}

static void apply_calibration(soil_sensor_handle_t handle)
{
    config_t cfg = {0};
    if (app_context_get_config(&cfg) != ESP_OK) {
        return;
    }

    if (cfg.soil_adc_dry != 0U) {
        (void)soil_sensor_calibrate_dry(handle, cfg.soil_adc_dry);
    }
    if (cfg.soil_adc_wet != 0U) {
        (void)soil_sensor_calibrate_wet(handle, cfg.soil_adc_wet);
    }
}

static void update_context(const sensor_task_context_t *shared, uint8_t moisture)
{
    if (shared == NULL || shared->data == NULL) {
        return;
    }

    shared->data->soil_moisture = moisture;
}

/* =========================================================================
   SECTION: Task
   ========================================================================= */
static esp_err_t soil_read_impl(sensor_task_context_t *shared_ctx)
{
    if (shared_ctx == NULL) {
        ESP_LOGW(TAG, "shared context missing");
        return ESP_ERR_INVALID_ARG;
    }

    soil_sensor_handle_t handle = ensure_soil_sensor();
    if (handle == NULL) {
        ESP_LOGW(TAG, "soil sensor unavailable");
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "soil sensor handle=%p", (void *)handle);

    apply_calibration(handle);

    uint8_t moisture = 0;
    uint16_t raw = 0;
    if (soil_sensor_probe(handle, &raw, &moisture) != ESP_OK) {
        ESP_LOGW(TAG, "soil probe failed");
        return ESP_FAIL;
    }

    update_context(shared_ctx, moisture);
    config_t cfg = {0};
    if (app_context_get_config(&cfg) == ESP_OK) {
        ESP_LOGI(TAG, "soil raw=%u dry=%u wet=%u moisture=%u%%",
                 (unsigned)raw,
                 (unsigned)cfg.soil_adc_dry,
                 (unsigned)cfg.soil_adc_wet,
                 (unsigned)moisture);
    } else {
        ESP_LOGI(TAG, "soil raw=%u moisture=%u%%", (unsigned)raw, (unsigned)moisture);
    }
    return ESP_OK;
}

/* =========================================================================
   SECTION: Public API
   ========================================================================= */
esp_err_t soil_sensor_read_once(sensor_task_context_t *shared_ctx)
{
    return soil_read_impl(shared_ctx);
}

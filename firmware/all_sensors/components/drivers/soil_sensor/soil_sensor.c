#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "soil_sensor.h"

/* =========================================================================
   SECTION: Types & Constants
   ========================================================================= */
#define SOIL_SENSOR_DEFAULT_SAMPLES   8
#define SOIL_SENSOR_DEFAULT_SETTLE_MS 50

struct soil_sensor {
    soil_sensor_config_t cfg;
    adc_oneshot_unit_handle_t unit;
    bool enabled;
    uint16_t cal_dry;
    uint16_t cal_wet;
    bool cal_dry_set;
    bool cal_wet_set;
};

static const char *TAG = "SOIL_SENSOR";

/* =========================================================================
   SECTION: Helpers
   ========================================================================= */
static bool gpio_is_valid(gpio_num_t gpio)
{
    return gpio >= 0 && gpio < GPIO_NUM_MAX;
}

static void soil_sensor_power_set(struct soil_sensor *s, bool on)
{
    if (!gpio_is_valid(s->cfg.power_gpio)) {
        return;
    }
    gpio_set_level(s->cfg.power_gpio, s->cfg.power_active_high ? (on ? 1 : 0) : (on ? 0 : 1));
}

static esp_err_t soil_sensor_configure_power_pin(struct soil_sensor *s)
{
    if (!gpio_is_valid(s->cfg.power_gpio)) {
        return ESP_OK;
    }
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << s->cfg.power_gpio,
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&io), TAG, "power gpio config failed");
    soil_sensor_power_set(s, false);
    return ESP_OK;
}

static esp_err_t soil_sensor_setup_adc(struct soil_sensor *s)
{
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = s->cfg.unit,
    };
    ESP_RETURN_ON_ERROR(adc_oneshot_new_unit(&unit_cfg, &s->unit), TAG, "adc unit create");

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = s->cfg.bitwidth,
        .atten = s->cfg.atten,
    };
    ESP_RETURN_ON_ERROR(adc_oneshot_config_channel(s->unit, s->cfg.channel, &chan_cfg), TAG, "adc chan cfg");
    s->enabled = true;
    return ESP_OK;
}

static esp_err_t soil_sensor_teardown_adc(struct soil_sensor *s)
{
    if (s->unit) {
        ESP_RETURN_ON_ERROR(adc_oneshot_del_unit(s->unit), TAG, "adc unit del");
        s->unit = NULL;
    }
    s->enabled = false;
    return ESP_OK;
}

static uint8_t clamp_u8(int value)
{
    if (value < 0) {
        return 0;
    }
    if (value > 100) {
        return 100;
    }
    return (uint8_t)value;
}

/* =========================================================================
   SECTION: Public API
   ========================================================================= */
esp_err_t soil_sensor_create(const soil_sensor_config_t *cfg, soil_sensor_handle_t *out_handle)
{
    ESP_RETURN_ON_FALSE(cfg != NULL, ESP_ERR_INVALID_ARG, TAG, "cfg null");
    ESP_RETURN_ON_FALSE(out_handle != NULL, ESP_ERR_INVALID_ARG, TAG, "out null");

    struct soil_sensor *s = calloc(1, sizeof(struct soil_sensor));
    ESP_RETURN_ON_FALSE(s != NULL, ESP_ERR_NO_MEM, TAG, "no mem");

    s->cfg = *cfg;
    if (s->cfg.sample_count == 0) {
        s->cfg.sample_count = SOIL_SENSOR_DEFAULT_SAMPLES;
    }
    if (s->cfg.settle_ms == 0) {
        s->cfg.settle_ms = SOIL_SENSOR_DEFAULT_SETTLE_MS;
    }
    s->cal_wet = 0;

    esp_err_t err = soil_sensor_configure_power_pin(s);
    if (err != ESP_OK) {
        free(s);
        return err;
    }

    *out_handle = s;
    return ESP_OK;
}

void soil_sensor_destroy(soil_sensor_handle_t handle)
{
    if (handle == NULL) {
        return;
    }
    struct soil_sensor *s = handle;
    if (s->enabled) {
        (void)soil_sensor_disable(handle);
    }
    free(s);
}

esp_err_t soil_sensor_enable(soil_sensor_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "handle null");
    struct soil_sensor *s = handle;
    if (s->enabled) {
        return ESP_OK;
    }
    soil_sensor_power_set(s, true);
    if (s->cfg.settle_ms) {
        vTaskDelay(pdMS_TO_TICKS(s->cfg.settle_ms));
    }
    return soil_sensor_setup_adc(s);
}

esp_err_t soil_sensor_disable(soil_sensor_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "handle null");
    struct soil_sensor *s = handle;
    ESP_RETURN_ON_ERROR(soil_sensor_teardown_adc(s), TAG, "adc teardown");
    soil_sensor_power_set(s, false);
    return ESP_OK;
}

esp_err_t soil_sensor_read_raw(soil_sensor_handle_t handle, uint16_t *out_raw)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "handle null");
    ESP_RETURN_ON_FALSE(out_raw != NULL, ESP_ERR_INVALID_ARG, TAG, "out null");
    struct soil_sensor *s = handle;
    ESP_RETURN_ON_FALSE(s->enabled, ESP_ERR_INVALID_STATE, TAG, "sensor disabled");

    uint32_t acc = 0;
    for (uint8_t i = 0; i < s->cfg.sample_count; i++) {
        int val = 0;
        ESP_RETURN_ON_ERROR(adc_oneshot_read(s->unit, s->cfg.channel, &val), TAG, "adc read");
        acc += (uint32_t)val;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    uint16_t avg = (uint16_t)(acc / s->cfg.sample_count);
    *out_raw = avg;
    return ESP_OK;
}

esp_err_t soil_sensor_calibrate_dry(soil_sensor_handle_t handle, uint16_t raw)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "handle null");
    struct soil_sensor *s = handle;
    s->cal_dry = raw;
    s->cal_dry_set = true;
    return ESP_OK;
}

esp_err_t soil_sensor_calibrate_wet(soil_sensor_handle_t handle, uint16_t raw)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "handle null");
    struct soil_sensor *s = handle;
    s->cal_wet = raw;
    s->cal_wet_set = true;
    return ESP_OK;
}

esp_err_t soil_sensor_compute_moisture(soil_sensor_handle_t handle, uint16_t raw, uint8_t *out_pct)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "handle null");
    ESP_RETURN_ON_FALSE(out_pct != NULL, ESP_ERR_INVALID_ARG, TAG, "out null");
    struct soil_sensor *s = handle;
    ESP_RETURN_ON_FALSE(s->cal_dry_set && s->cal_wet_set, ESP_ERR_INVALID_STATE, TAG, "cal missing");
    ESP_RETURN_ON_FALSE(s->cal_wet != s->cal_dry, ESP_ERR_INVALID_STATE, TAG, "cal identical");

    int diff = (int)s->cal_wet - (int)s->cal_dry;
    int rel = ((int)raw - (int)s->cal_dry) * 100 / diff;
    *out_pct = clamp_u8(rel);
    return ESP_OK;
}

esp_err_t soil_sensor_probe(soil_sensor_handle_t handle, uint16_t *out_raw, uint8_t *out_pct)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "handle null");
    struct soil_sensor *s = handle;

    bool was_enabled = s->enabled;
    if (!was_enabled) {
        ESP_RETURN_ON_ERROR(soil_sensor_enable(handle), TAG, "enable");
    }

    uint16_t raw = 0;
    esp_err_t err = soil_sensor_read_raw(handle, &raw);

    if (!was_enabled) {
        (void)soil_sensor_disable(handle);
    }
    if (err != ESP_OK) {
        return err;
    }

    if (out_raw) {
        *out_raw = raw;
    }
    if (out_pct) {
        err = soil_sensor_compute_moisture(handle, raw, out_pct);
    }
    return err;
}

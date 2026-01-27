#include <string.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "board_pins.h"
#include "ssd1306.h"
#include "fsm_manager.h"
#include "nvs_manager.h"
#include "soil_sensor.h"
#include "app_context.h"
#include "fsm_state_callbacks.h"

static const char *TAG = "STATE_CALIB_DRY";
static const uint64_t CALIB_TIMEOUT_US = 30ULL * 1000ULL * 1000ULL;

static esp_timer_handle_t s_timer;
static soil_sensor_handle_t s_soil;


static soil_sensor_handle_t ensure_soil_sensor(void)
{
    if (s_soil != NULL) {
        return s_soil;
    }

    soil_sensor_handle_t h_ctx = app_context_get_soil_sensor();
    if (h_ctx != NULL) {
        s_soil = h_ctx;
        return s_soil;
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

    if (soil_sensor_create(&cfg, &s_soil) != ESP_OK) {
        s_soil = NULL;
        return NULL;
    }
    (void)app_context_set_soil_sensor(s_soil);
    return s_soil;
}

static void calib_dry_display_show(void)
{
    ssd1306_handle_t disp = app_context_ensure_display();
    if (disp == NULL) {
        return;
    }
    (void)ssd1306_clear(disp);
    (void)ssd1306_draw_text(disp, "CALIBRATION", 2, 0);
    (void)ssd1306_draw_text(disp, "Press the right button, when the soil moisture sensor is DRY.", 0, 2);
    (void)ssd1306_draw_button_label_right(disp, "NEXT");
    (void)ssd1306_flush(disp);
}

static void calib_dry_timer_stop(void)
{
    if (s_timer) {
        (void)esp_timer_stop(s_timer);
        (void)esp_timer_delete(s_timer);
        s_timer = NULL;
    }
}

static void calib_dry_timeout_cb(void *arg)
{
    (void)arg;
    (void)fsm_manager_post_event(APP_EVENT_CALIB_TIMEOUT, NULL, 0, 0);
}

static void calib_dry_timer_start(void)
{
    if (s_timer != NULL) {
        return;
    }

    const esp_timer_create_args_t args = {
        .callback = calib_dry_timeout_cb,
        .name = "calib_dry_timeout",
    };

    if (esp_timer_create(&args, &s_timer) != ESP_OK) {
        ESP_LOGW(TAG, "timer create failed");
        return;
    }

    if (esp_timer_start_once(s_timer, CALIB_TIMEOUT_US) != ESP_OK) {
        ESP_LOGW(TAG, "timer start failed");
        calib_dry_timer_stop();
    }
}

/* =========================================================================
   SECTION: Callbacks
   ========================================================================= */
void state_calib_soil_dry_on_enter(void)
{
    calib_dry_display_show();
    calib_dry_timer_start();

    soil_sensor_handle_t soil = ensure_soil_sensor();
    if (soil == NULL) {
        ESP_LOGW(TAG, "soil sensor unavailable");
        return;
    }
}

void state_calib_soil_dry_on_exit(exit_mode_t mode)
{
    calib_dry_timer_stop();

    if (mode != EXIT_MODE_DEFAULT) {
        return;
    }

    soil_sensor_handle_t soil = ensure_soil_sensor();
    if (soil == NULL) {
        ESP_LOGW(TAG, "soil sensor unavailable");
        return;
    }

    uint16_t raw = 0;
    if (soil_sensor_probe(soil, &raw, NULL) != ESP_OK) {
        ESP_LOGW(TAG, "soil probe failed on exit");
        return;
    }

    ESP_LOGI(TAG, "calibration dry raw=%u", (unsigned)raw);
    (void)soil_sensor_calibrate_dry(soil, raw);

    config_t cfg = {0};
    if (app_context_get_config(&cfg) != ESP_OK) {
        memset(&cfg, 0, sizeof(cfg));
    }
    cfg.soil_adc_dry = raw;
    (void)app_context_set_config(&cfg);

    esp_err_t err = nvs_manager_save_config(&cfg);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "save dry calib failed (%s)", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "saved dry calib raw=%u", (unsigned)raw);
    }
}

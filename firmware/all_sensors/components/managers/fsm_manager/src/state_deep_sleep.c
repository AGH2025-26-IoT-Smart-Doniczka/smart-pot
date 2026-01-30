#include <time.h>
#include <string.h>
#include "esp_log.h"
#include "esp_sleep.h"
#include "bsp_init.h"
#include "ssd1306.h"
#include "app_context.h"
#include "app_constants.h"
#include "buttons_manager.h"
#include "fsm_state_callbacks.h"
#include "watering_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "STATE_SLEEP";

static bool config_allows_timer_wakeup(const config_t *cfg)
{
    if (cfg == NULL) {
        ESP_LOGW(TAG, "config invalid: null");
        return false;
    }

    size_t ssid_len = strnlen(cfg->ssid, sizeof(cfg->ssid));
    if (ssid_len == 0U || ssid_len >= MAX_SSID_LENGTH) {
        ESP_LOGW(TAG, "config invalid: ssid_len=%lu", (unsigned long)ssid_len);
        return false;
    }

    size_t pass_len = strnlen(cfg->passwd, sizeof(cfg->passwd));
    if (pass_len > MAX_PASSWD_LENGTH) {
        ESP_LOGW(TAG, "config invalid: wifi password len=%lu", (unsigned long)pass_len);
        return false;
    }


    if (cfg->soil_adc_dry == 0U || cfg->soil_adc_wet == 0U) {
        ESP_LOGW(TAG, "config invalid: soil cal missing dry=%u wet=%u",
                 (unsigned)cfg->soil_adc_dry, (unsigned)cfg->soil_adc_wet);
        return false;
    }

    if (cfg->soil_adc_wet >= cfg->soil_adc_dry) {
        ESP_LOGW(TAG, "config invalid: soil cal order dry=%u wet=%u",
                 (unsigned)cfg->soil_adc_dry, (unsigned)cfg->soil_adc_wet);
        return false;
    }

    return true;
}

/* =========================================================================
   SECTION: Callbacks
   ========================================================================= */
void state_deep_sleep_on_enter(void)
{
    ESP_LOGI(TAG, "enter");

    watering_manager_deinit();

    config_t cfg = {0};
    if (app_context_get_config(&cfg) != ESP_OK) {
        memset(&cfg, 0, sizeof(cfg));
    }

    bool allow_timer_wakeup = config_allows_timer_wakeup(&cfg);

    uint32_t sleep_s = cfg.mes;
    if (sleep_s == 0U) {
        sleep_s = 60U;
    }

    ssd1306_handle_t disp = app_context_get_display_handle();
    i2c_master_bus_handle_t bus = app_context_get_display_bus();

    if (disp) {
        (void)ssd1306_power_off(disp);
        ssd1306_destroy(disp);
        (void)app_context_set_display_handle(NULL);
    }
    if (bus) {
        (void)bsp_i2c_del_bus(bus);
        (void)app_context_set_display_bus(NULL);
    }

    if (allow_timer_wakeup) {
        ESP_LOGI(TAG, "deep sleep %us", (unsigned)sleep_s);
    } else {
        ESP_LOGW(TAG, "deep sleep: timer wakeup disabled (invalid config)");
    }
    esp_err_t err = esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "disable wake sources failed (%s)", esp_err_to_name(err));
    }
    err = buttons_manager_enable_deep_sleep_wakeup();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "btn1 wakeup enable failed (%s)", esp_err_to_name(err));
    }
    if (allow_timer_wakeup) {
        err = esp_sleep_enable_timer_wakeup((uint64_t)sleep_s * 1000000ULL);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "timer wakeup failed (%s)", esp_err_to_name(err));
        }
    }
    esp_deep_sleep_start();
}

void state_deep_sleep_on_exit(exit_mode_t mode)
{
    (void)mode;
    ESP_LOGI(TAG, "exit");
}

#include <string.h>
#include <time.h>
#include "esp_log.h"
#include "esp_sleep.h"
#include "nvs_manager.h"
#include "bsp_init.h"
#include "ssd1306.h"
#include "fsm_manager.h"
#include "app_context.h"
#include "fsm_state_callbacks.h"

static const char *TAG = "STATE_INIT";

static void maybe_teardown_display(exit_mode_t mode)
{
    // Keep display alive on normal transitions; only tear down when interrupted.
    if (mode == EXIT_MODE_DEFAULT) {
        return;
    }

    ssd1306_handle_t disp = app_context_get_display_handle();
    i2c_master_bus_handle_t bus = app_context_get_display_bus();

    if (disp) {
        ssd1306_destroy(disp);
        (void)app_context_set_display_handle(NULL);
    }
    if (bus) {
        (void)bsp_i2c_del_bus(bus);
        (void)app_context_set_display_bus(NULL);
    }
}


static void log_config_on_boot(const config_t *cfg, bool has_cfg)
{
    if (cfg == NULL) {
        return;
    }

    const char *ssid = (cfg->ssid[0] != '\0') ? cfg->ssid : "<empty>";

    ESP_LOGI(TAG, "cfg.present=%u", has_cfg ? 1U : 0U);
    ESP_LOGI(TAG, "cfg.ssid=%s", ssid);
    ESP_LOGI(TAG, "cfg.plant.moi_min=%u moi_max=%u", (unsigned)cfg->plant_config.moi[0], (unsigned)cfg->plant_config.moi[1]);
    ESP_LOGI(TAG, "cfg.plant.tem_min=%u tem_max=%u", (unsigned)cfg->plant_config.tem[0], (unsigned)cfg->plant_config.tem[1]);
    ESP_LOGI(TAG, "cfg.mes=%u sen=%u wat=%u wai=%u",
             (unsigned)cfg->mes,
             (unsigned)cfg->sen,
             (unsigned)cfg->wat,
             (unsigned)cfg->wai);
    ESP_LOGI(TAG, "cfg.soil_adc_dry=%u soil_adc_wet=%u",
             (unsigned)cfg->soil_adc_dry,
             (unsigned)cfg->soil_adc_wet);
    size_t mqtt_len = strnlen(cfg->mqtt_passwd, sizeof(cfg->mqtt_passwd));
    ESP_LOGI(TAG, "cfg.mqtt_pass_len=%lu", (unsigned long)mqtt_len);
    char mqtt_hex[sizeof(cfg->mqtt_passwd) * 2U + 1U] = {0};
    for (size_t i = 0; i < mqtt_len; ++i) {
        (void)snprintf(&mqtt_hex[i * 2U], 3U, "%02X", (unsigned char)cfg->mqtt_passwd[i]);
    }
    ESP_LOGI(TAG, "cfg.mqtt_pass_hex=%s", mqtt_hex);
}

/* =========================================================================
   SECTION: Callbacks
   ========================================================================= */
void state_init_on_enter(void)
{
    esp_sleep_wakeup_cause_t wake = esp_sleep_get_wakeup_cause();
    time_t now = time(NULL);
    ESP_LOGI(TAG, "wakeup reason=%d ts=%ld", (int)wake, (long)now);

    if (wake == ESP_SLEEP_WAKEUP_EXT0 || wake == ESP_SLEEP_WAKEUP_EXT1) {
        app_context_set_force_sync_on_wakeup(true);
        app_context_set_display_on_wakeup(true);
        ESP_LOGI(TAG, "force sync on wakeup=1");
    } else {
        app_context_set_force_sync_on_wakeup(false);
        app_context_set_display_on_wakeup(false);
    }

    ssd1306_handle_t disp = app_context_ensure_display();
    if (disp != NULL) {
        (void)ssd1306_clear(disp);
        (void)ssd1306_draw_text(disp, ".", 0, 0);
        (void)ssd1306_flush(disp);
    }
    config_t cfg = {0};
    bool has_cfg = false;
    esp_err_t err = nvs_manager_load_config(&cfg, &has_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "load config failed (%s)", esp_err_to_name(err));
        (void)fsm_manager_post_event(APP_EVENT_NO_CONFIG, NULL, 0, 0);
        return;
    }

    log_config_on_boot(&cfg, has_cfg);

    bool has_prior_connect = false;
    if (nvs_manager_get_first_connect(&has_prior_connect) != ESP_OK) {
        has_prior_connect = false;
    }
    app_context_set_has_prior_connect(has_prior_connect);
    ESP_LOGI(TAG, "boot.first_connect_done=%u", has_prior_connect ? 1U : 0U);

    if (has_cfg) {
        (void)app_context_set_config(&cfg);
        size_t ssid_len = strnlen(cfg.ssid, sizeof(cfg.ssid));
        ESP_LOGI(TAG, "wifi ssid=%.*s",
             (int)ssid_len, cfg.ssid);
        (void)fsm_manager_post_event(APP_EVENT_CONFIG_LOADED, NULL, 0, 0);
    } else {
        const bool soil_cal_ok = (cfg.soil_adc_dry != 0U) &&
                                 (cfg.soil_adc_wet != 0U) &&
                                 (cfg.soil_adc_wet < cfg.soil_adc_dry);
        if (soil_cal_ok) {
            (void)app_context_set_config(&cfg);
            (void)fsm_manager_post_event(APP_EVENT_NEEDS_PROVISIONING, NULL, 0, 0);
        } else {
            (void)fsm_manager_post_event(APP_EVENT_NO_CONFIG, NULL, 0, 0);
        }
    }
}

void state_init_on_exit(exit_mode_t mode)
{
    maybe_teardown_display(mode);
}

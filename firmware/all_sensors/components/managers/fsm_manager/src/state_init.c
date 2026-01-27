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

static void init_display_show(const char *line1, const char *line2)
{
    ssd1306_handle_t disp = app_context_ensure_display();
    if (disp == NULL) {
        return;
    }

    (void)ssd1306_clear(disp);
    if (line1 != NULL) {
        (void)ssd1306_draw_text(disp, line1, 0, 0);
    }
    if (line2 != NULL) {
        (void)ssd1306_draw_text(disp, line2, 0, 2);
    }
    (void)ssd1306_flush(disp);
}

/* =========================================================================
   SECTION: Callbacks
   ========================================================================= */
void state_init_on_enter(void)
{
    time_t now = time(NULL);
    ESP_LOGI(TAG, "wakeup reason=%d ts=%ld", (int)esp_sleep_get_wakeup_cause(), (long)now);

    init_display_show("INIT", NULL);

    config_t cfg = {0};
    bool has_cfg = false;
    esp_err_t err = nvs_manager_load_config(&cfg, &has_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "load config failed (%s)", esp_err_to_name(err));
        init_display_show("INIT", "CFG ERR");
        (void)fsm_manager_post_event(APP_EVENT_NO_CONFIG, NULL, 0, 0);
        return;
    }

    if (has_cfg) {
        (void)app_context_set_config(&cfg);
        size_t ssid_len = strnlen(cfg.ssid, sizeof(cfg.ssid));
        size_t pass_len = strnlen(cfg.passwd, sizeof(cfg.passwd));
        ESP_LOGI(TAG, "wifi ssid=%.*s passwd=%.*s",
             (int)ssid_len, cfg.ssid,
             (int)pass_len, cfg.passwd);
        init_display_show("INIT", "CFG LOADED");
        (void)fsm_manager_post_event(APP_EVENT_CONFIG_LOADED, NULL, 0, 0);
    } else {
        init_display_show("INIT", "NO CFG");
        (void)fsm_manager_post_event(APP_EVENT_NO_CONFIG, NULL, 0, 0);
    }
}

void state_init_on_exit(exit_mode_t mode)
{
    maybe_teardown_display(mode);
}

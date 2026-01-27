#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "board_pins.h"
#include "ssd1306.h"
#include "soil_sensor.h"
#include "app_context.h"
#include "ble_provisioning.h"
#include "fsm_state_callbacks.h"

static const char *TAG = "STATE_PROV";

/* =========================================================================
   SECTION: Helpers
   ========================================================================= */
static bool soil_calibration_missing(void)
{
    config_t cfg = {0};
    if (app_context_get_config(&cfg) != ESP_OK) {
        memset(&cfg, 0, sizeof(cfg));
    }

    if (cfg.soil_adc_dry == 0U || cfg.soil_adc_wet == 0U) {
        return true;
    }

    return (cfg.soil_adc_wet >= cfg.soil_adc_dry);
}

static void display_show_pairing(void)
{
    ssd1306_handle_t disp = app_context_ensure_display();
    if (disp == NULL) {
        return;
    }

    (void)ssd1306_clear(disp);
    (void)ssd1306_draw_text(disp, "PAIRING", 0, 0);
    (void)ssd1306_draw_text(disp, "USE APP", 0, 2);
    (void)ssd1306_flush(disp);
}

/* =========================================================================
   SECTION: Callbacks
   ========================================================================= */
void state_provisioning_on_enter(void)
{
    ESP_LOGI(TAG, "enter");
    if (soil_calibration_missing()) {
        ESP_LOGW(TAG, "soil calibration missing; redirecting");
        (void)fsm_manager_post_event(APP_EVENT_REQUIRES_CALIBRATION, NULL, 0, 0);
        return;
    }
    display_show_pairing();
    (void)ble_provisioning_start();
}

void state_provisioning_on_exit(exit_mode_t mode)
{
    (void)mode;
    ble_provisioning_stop();
    ESP_LOGI(TAG, "exit");
}

#include <time.h>
#include "esp_log.h"
#include "esp_sleep.h"
#include "bsp_init.h"
#include "ssd1306.h"
#include "app_context.h"
#include "buttons_manager.h"
#include "fsm_state_callbacks.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "STATE_SLEEP";

/* =========================================================================
   SECTION: Callbacks
   ========================================================================= */
void state_deep_sleep_on_enter(void)
{
    ESP_LOGI(TAG, "enter");

   

    config_t cfg = {0};
    if (app_context_get_config(&cfg) != ESP_OK) {
        memset(&cfg, 0, sizeof(cfg));
    }

    uint32_t sleep_s = cfg.sleep_duration;
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

    ESP_LOGI(TAG, "deep sleep %us", (unsigned)sleep_s);
    esp_err_t err = esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "disable wake sources failed (%s)", esp_err_to_name(err));
    }
    err = buttons_manager_enable_deep_sleep_wakeup();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "btn1 wakeup enable failed (%s)", esp_err_to_name(err));
    }
    err = esp_sleep_enable_timer_wakeup((uint64_t)sleep_s * 1000000ULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "timer wakeup failed (%s)", esp_err_to_name(err));
    }
    esp_deep_sleep_start();
}

void state_deep_sleep_on_exit(exit_mode_t mode)
{
    (void)mode;
    ESP_LOGI(TAG, "exit");
}

#include "esp_log.h"
#include "esp_system.h"
#include <nvs_flash.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "app_context.h"
#include "mqtt_manager.h"
#include "fsm_state_callbacks.h"


static const char *TAG = "STATE_RESET";

esp_err_t erase_config() {
    nvs_flash_erase();
    return ESP_OK;
}



/* =========================================================================
   SECTION: Callbacks
   ========================================================================= */
void state_factory_reset_on_enter(void)
{
    ESP_LOGW(TAG, "factory reset: erasing flash");

    if (app_context_is_wifi_connected()) {
        (void)mqtt_manager_start();
        const int64_t deadline_us = esp_timer_get_time() + (5LL * 1000LL * 1000LL);
        while (esp_timer_get_time() < deadline_us) {
            if (mqtt_manager_publish_log_sync("factory_reset", 2, "Factory reset initiated", 1000) == ESP_OK) {
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }

    esp_err_t err = erase_config();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "erase failed: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "erase complete; rebooting");
    }

    esp_restart();
}

void state_factory_reset_on_exit(exit_mode_t mode)
{
    (void)mode;
    ESP_LOGI(TAG, "exit");
}

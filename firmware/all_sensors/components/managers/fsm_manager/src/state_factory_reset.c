#include "esp_log.h"
#include "esp_system.h"
#include <nvs_flash.h>
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

#include "esp_log.h"
#include "wifi_manager.h"
#include "fsm_state_callbacks.h"

static const char *TAG = "STATE_WIFI";

/* =========================================================================
   SECTION: Callbacks
   ========================================================================= */
void state_wifi_connect_on_enter(void)
{
    ESP_LOGI(TAG, "enter");
    (void)wifi_manager_start();
}

void state_wifi_connect_on_exit(exit_mode_t mode)
{
    if (mode == EXIT_MODE_INTERRUPTED) {
        wifi_manager_stop();
    }
    ESP_LOGI(TAG, "exit");
}

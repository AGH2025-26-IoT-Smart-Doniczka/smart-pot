#include "esp_log.h"
#include "wifi_manager.h"
#include "fsm_state_callbacks.h"

static const char *TAG = "STATE_SYNC";

/* =========================================================================
   SECTION: Callbacks
   ========================================================================= */
void state_sync_time_on_enter(void)
{
    ESP_LOGI(TAG, "enter");
    (void)wifi_manager_request_time_sync();
}

void state_sync_time_on_exit(exit_mode_t mode)
{
    (void)mode;
    ESP_LOGI(TAG, "exit");
}

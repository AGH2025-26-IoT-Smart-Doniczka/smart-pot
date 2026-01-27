#include "esp_log.h"
#include "app_context.h"
#include "fsm_manager.h"
#include "fsm_state_callbacks.h"

static const char *TAG = "STATE_DECISION";

/* =========================================================================
   SECTION: Callbacks
   ========================================================================= */
void state_data_decision_on_enter(void)
{
    ESP_LOGI(TAG, "enter");

    if (app_context_is_wifi_connected()) {
        ESP_LOGI(TAG, "wifi connected -> mqtt");
        (void)fsm_manager_post_event(APP_EVENT_DECISION_MQTT, NULL, 0, 0);
        return;
    }

    ESP_LOGI(TAG, "wifi offline -> storage");
    (void)fsm_manager_post_event(APP_EVENT_DECISION_STORAGE, NULL, 0, 0);
}

void state_data_decision_on_exit(exit_mode_t mode)
{
    (void)mode;
    ESP_LOGI(TAG, "exit");
}

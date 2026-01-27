#include "esp_log.h"
#include "fsm_manager.h"
#include "fsm_state_callbacks.h"
#include "mqtt_manager.h"

static const char *TAG = "STATE_MQTT";

/* =========================================================================
   SECTION: Callbacks
   ========================================================================= */
void state_mqtt_publish_on_enter(void)
{
    ESP_LOGI(TAG, "enter");
    if (mqtt_manager_publish_telemetry() != ESP_OK) {
        ESP_LOGW(TAG, "mqtt publish failed");
        (void)fsm_manager_post_event(APP_EVENT_WIFI_DISCONNECTED, NULL, 0, 0);
        return;
    }
}

void state_mqtt_publish_on_exit(exit_mode_t mode)
{
    (void)mode;
    ESP_LOGI(TAG, "exit");
}

#include <time.h>
#include "esp_log.h"
#include "app_context.h"
#include "nvs_manager.h"
#include "fsm_manager.h"
#include "fsm_state_callbacks.h"

static const char *TAG = "STATE_STORE";

/* =========================================================================
   SECTION: Callbacks
   ========================================================================= */
void state_flash_store_on_enter(void)
{
    ESP_LOGI(TAG, "enter");

    sensor_data_t data = {0};
    (void)app_context_get_sensor_data(&data);

    sensor_sample_t sample = {0};
    sample.data = data;
    if (app_context_is_time_synced()) {
        sample.timestamp = (uint32_t)time(NULL);
    }

    esp_err_t err = nvs_manager_store_sample(&sample);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "store sample failed (%s)", esp_err_to_name(err));
    }

    (void)fsm_manager_post_event(APP_EVENT_STORAGE_SAVED, NULL, 0, 0);
}

void state_flash_store_on_exit(exit_mode_t mode)
{
    (void)mode;
    ESP_LOGI(TAG, "exit");
}

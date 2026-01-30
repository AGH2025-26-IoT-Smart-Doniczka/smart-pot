#include <time.h>
#include "esp_log.h"
#include "esp_attr.h"
#include "app_context.h"
#include "fsm_manager.h"
#include "fsm_state_callbacks.h"

static const char *TAG = "STATE_DEC_SYNC";

/* =========================================================================
   SECTION: RTC State
   ========================================================================= */
RTC_DATA_ATTR static uint32_t s_last_sync_s = 0;

/* =========================================================================
   SECTION: Callbacks
   ========================================================================= */
void state_decision_sync_on_enter(void)
{
    ESP_LOGI(TAG, "enter");

    config_t cfg = {0};
    if (app_context_get_config(&cfg) != ESP_OK) {
        ESP_LOGW(TAG, "config unavailable");
        (void)fsm_manager_post_event(APP_EVENT_DECISION_SYNC_STORAGE, NULL, 0, 0);
        return;
    }

    uint32_t now_s = (uint32_t)time(NULL);
    uint32_t interval_s = (uint32_t)cfg.sen;
    bool force_sync = app_context_force_sync_on_wakeup();
    bool should_sync = force_sync ||
                       (s_last_sync_s < 1000U) ||
                       (now_s >= s_last_sync_s && (now_s - s_last_sync_s) >= interval_s);

    ESP_LOGI(TAG, "sync check: last=%u now=%u mes=%u elapsed=%u force=%u should_sync=%u",
             (unsigned)s_last_sync_s,
             (unsigned)now_s,
             (unsigned)interval_s,
             (unsigned)((now_s >= s_last_sync_s) ? (now_s - s_last_sync_s) : 0U),
             force_sync ? 1U : 0U,
             should_sync ? 1U : 0U);

    if (should_sync) {
        ESP_LOGI(TAG, "sync due (last=%u now=%u mes=%u)",
                 (unsigned)s_last_sync_s, (unsigned)now_s, (unsigned)interval_s);
        s_last_sync_s = now_s;
        if (force_sync) {
            app_context_set_force_sync_on_wakeup(false);
        }
        (void)fsm_manager_post_event(APP_EVENT_DECISION_SYNC_WIFI, NULL, 0, 0);
        return;
    }

    ESP_LOGI(TAG, "sync not due (last=%u now=%u mes=%u)",
             (unsigned)s_last_sync_s, (unsigned)now_s, (unsigned)interval_s);
    (void)fsm_manager_post_event(APP_EVENT_DECISION_SYNC_STORAGE, NULL, 0, 0);
}

void state_decision_sync_on_exit(exit_mode_t mode)
{
    (void)mode;
    ESP_LOGI(TAG, "exit");
}

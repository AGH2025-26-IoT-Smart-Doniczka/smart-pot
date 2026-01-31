#include <time.h>
#include "esp_err.h"
#include "esp_log.h"
#include "app_context.h"
#include "fsm_manager.h"
#include "fsm_state_callbacks.h"
#include "watering_manager.h"

static const char *TAG = "STATE_DEC_WATER";

/* =========================================================================
   SECTION: Helpers
   ========================================================================= */
static uint32_t clamp_watering_duration(uint16_t wat_s)
{
    if (wat_s == 0U) {
        return 0U;
    }
    return (wat_s > 60U) ? 60U : (uint32_t)wat_s;
}

static const char *soil_status_str(soil_status_t status)
{
    switch (status) {
        case SOIL_STATUS_TOO_DRY: return "TOO_DRY";
        case SOIL_STATUS_OK: return "OK";
        case SOIL_STATUS_TOO_WET: return "TOO_WET";
        case SOIL_STATUS_UNKNOWN: return "UNKNOWN";
        default: return "UNKNOWN";
    }
}

/* =========================================================================
   SECTION: Callbacks
   ========================================================================= */
void state_decision_water_on_enter(void)
{
    ESP_LOGI(TAG, "enter");

    config_t cfg = {0};
    if (app_context_get_config(&cfg) != ESP_OK) {
        ESP_LOGW(TAG, "config unavailable");
        (void)fsm_manager_post_event(APP_EVENT_DECISION_WATER_DONE, NULL, 0, 0);
        return;
    }

    uint32_t now_s = (uint32_t)time(NULL);
    uint32_t due_after_s = (uint32_t)cfg.wai;
    uint32_t duration_s = clamp_watering_duration(cfg.wat);
    soil_status_t soil_status = app_context_get_soil_status();

    ESP_LOGI(TAG, "soil status=%s", soil_status_str(soil_status));

    if (soil_status == SOIL_STATUS_TOO_WET || soil_status == SOIL_STATUS_UNKNOWN) {
        ESP_LOGI(TAG, "watering blocked (soil=%s)", soil_status_str(soil_status));
        (void)fsm_manager_post_event(APP_EVENT_DECISION_WATER_DONE, NULL, 0, 0);
        return;
    }

    if (duration_s == 0U) {
        ESP_LOGI(TAG, "watering skipped (wat=%u)", (unsigned)cfg.wat);
        (void)fsm_manager_post_event(APP_EVENT_DECISION_WATER_DONE, NULL, 0, 0);
        return;
    }

    if (soil_status == SOIL_STATUS_TOO_DRY) {
        ESP_LOGI(TAG, "soil too dry -> watering now (dur=%u)", (unsigned)duration_s);
        esp_err_t err = watering_manager_start_blocking((uint16_t)duration_s);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "watering failed (%s)", esp_err_to_name(err));
        }
        (void)fsm_manager_post_event(APP_EVENT_DECISION_WATER_DONE, NULL, 0, 0);
        return;
    }

    if (due_after_s == 0U) {
        ESP_LOGI(TAG, "watering skipped (wai=%u)", (unsigned)cfg.wai);
        (void)fsm_manager_post_event(APP_EVENT_DECISION_WATER_DONE, NULL, 0, 0);
        return;
    }

    uint32_t last_watering_s = watering_manager_get_last_watering_s();
    bool should_water = (last_watering_s == 0U) ||
                        (now_s >= last_watering_s && (now_s - last_watering_s) >= due_after_s);

    if (should_water) {
        ESP_LOGI(TAG, "watering due (last=%u now=%u dur=%u)",
                 (unsigned)last_watering_s, (unsigned)now_s, (unsigned)duration_s);
        esp_err_t err = watering_manager_start_blocking((uint16_t)duration_s);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "watering failed (%s)", esp_err_to_name(err));
        }
    } else {
        ESP_LOGI(TAG, "watering not due (last=%u now=%u wai=%u)",
                 (unsigned)last_watering_s, (unsigned)now_s, (unsigned)due_after_s);
    }

    (void)fsm_manager_post_event(APP_EVENT_DECISION_WATER_DONE, NULL, 0, 0);
}

void state_decision_water_on_exit(exit_mode_t mode)
{
    (void)mode;
    ESP_LOGI(TAG, "exit");
}

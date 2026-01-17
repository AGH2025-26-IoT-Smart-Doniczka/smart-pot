#include "fsm_state_callbacks.h"

/* =========================================================================
   SECTION: Callback Table
   ========================================================================= */
static const fsm_callbacks_t s_state_callbacks = {
    .init = {
        .on_enter = state_init_on_enter,
        .on_exit = state_init_on_exit,
    },
    .calib_soil_dry = {
        .on_enter = state_calib_soil_dry_on_enter,
        .on_exit = state_calib_soil_dry_on_exit,
    },
    .calib_soil_wet = {
        .on_enter = state_calib_soil_wet_on_enter,
        .on_exit = state_calib_soil_wet_on_exit,
    },
    .provisioning = {
        .on_enter = state_provisioning_on_enter,
        .on_exit = state_provisioning_on_exit,
    },
    .wifi_connect = {
        .on_enter = state_wifi_connect_on_enter,
        .on_exit = state_wifi_connect_on_exit,
    },
    .sync_time = {
        .on_enter = state_sync_time_on_enter,
        .on_exit = state_sync_time_on_exit,
    },
    .sensing = {
        .on_enter = state_sensing_on_enter,
        .on_exit = state_sensing_on_exit,
    },
    .data_decision = {
        .on_enter = state_data_decision_on_enter,
        .on_exit = state_data_decision_on_exit,
    },
    .mqtt_publish = {
        .on_enter = state_mqtt_publish_on_enter,
        .on_exit = state_mqtt_publish_on_exit,
    },
    .flash_store = {
        .on_enter = state_flash_store_on_enter,
        .on_exit = state_flash_store_on_exit,
    },
    .factory_reset = {
        .on_enter = state_factory_reset_on_enter,
        .on_exit = state_factory_reset_on_exit,
    },
    .idle = {
        .on_enter = state_idle_on_enter,
        .on_exit = state_idle_on_exit,
    },
    .deep_sleep = {
        .on_enter = state_deep_sleep_on_enter,
        .on_exit = state_deep_sleep_on_exit,
    },
};

/* =========================================================================
   SECTION: Public API
   ========================================================================= */
const fsm_callbacks_t *fsm_state_callbacks_get(void)
{
    return &s_state_callbacks;
}

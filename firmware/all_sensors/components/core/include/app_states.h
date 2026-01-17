#pragma once

/* =========================================================================
   State Enum
   ========================================================================= */
typedef enum {
    STATE_INIT = 0,

    STATE_CALIB_SOIL_DRY,
    STATE_CALIB_SOIL_WET,
    STATE_PROVISIONING,

    STATE_WIFI_CONNECT,
    STATE_SYNC_TIME,
    STATE_SENSING,          
    STATE_DATA_DECISION,   
    STATE_MQTT_PUBLISH,
    STATE_FLASH_STORE,

    STATE_FACTORY_RESET,
    STATE_IDLE,
    STATE_DEEP_SLEEP
} app_state_t;

/* =========================================================================
   Helpers
   ========================================================================= */
static inline const char* app_state_str(app_state_t s) {
    switch(s) {
        case STATE_INIT: return "INIT";
        case STATE_CALIB_SOIL_DRY: return "CAL_DRY";
        case STATE_CALIB_SOIL_WET: return "CAL_WET";
        case STATE_PROVISIONING: return "PROV";
        case STATE_WIFI_CONNECT: return "WIFI_CONN";
        case STATE_SYNC_TIME: return "SYNC";
        case STATE_SENSING: return "SENSING";
        case STATE_DATA_DECISION: return "Data+DECISION";
        case STATE_MQTT_PUBLISH: return "MQTT_PUBLISH";
        case STATE_FLASH_STORE: return "FLASH_STORE";
        case STATE_FACTORY_RESET: return "RESET";
        case STATE_IDLE: return "IDLE";
        case STATE_DEEP_SLEEP: return "SLEEP";
        default: return "UNKNOWN";
    }
}
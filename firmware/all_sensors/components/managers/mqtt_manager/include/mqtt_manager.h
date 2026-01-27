#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
#error "This project uses C only."
#endif

/* =========================================================================
   SECTION: API
   ========================================================================= */
// Initialize MQTT client if needed. Safe to call multiple times.
esp_err_t mqtt_manager_start(void);

// Publish telemetry once. Will start client if needed.
esp_err_t mqtt_manager_publish_telemetry(void);

// Stop MQTT client and prevent reconnects.
esp_err_t mqtt_manager_stop(void);

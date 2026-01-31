#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "app_types.h"

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

// Publish a logs message. Will start client if needed.
esp_err_t mqtt_manager_publish_log(const char *label, int level, const char *data);

// Publish a logs message and wait for PUBACK (best effort).
esp_err_t mqtt_manager_publish_log_sync(const char *label, int level, const char *data, uint32_t timeout_ms);

// Publish config sync log with full config payload.
esp_err_t mqtt_manager_publish_config_log(const config_t *cfg);

// Stop MQTT client and prevent reconnects.
esp_err_t mqtt_manager_stop(void);

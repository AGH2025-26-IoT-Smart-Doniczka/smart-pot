#pragma once

#include "esp_err.h"
#include "sensor_task_context.h"

#ifdef __cplusplus
#error "This project uses C only."
#endif

/* =========================================================================
   SECTION: API
   ========================================================================= */
esp_err_t bme280_read_once(sensor_task_context_t *shared_ctx);
esp_err_t bme280_debug_loop(sensor_task_context_t *shared_ctx, uint32_t interval_ms);

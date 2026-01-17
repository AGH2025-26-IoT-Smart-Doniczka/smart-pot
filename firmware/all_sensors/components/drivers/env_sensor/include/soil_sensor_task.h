#pragma once

#include "esp_err.h"
#include "sensor_task_context.h"

#ifdef __cplusplus
#error "This project uses C only."
#endif

/* =========================================================================
   SECTION: API
   ========================================================================= */
esp_err_t soil_sensor_read_once(sensor_task_context_t *shared_ctx);

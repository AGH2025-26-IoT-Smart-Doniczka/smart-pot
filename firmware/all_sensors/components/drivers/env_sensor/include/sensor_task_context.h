#pragma once

#include "driver/i2c_master.h"
#include "app_types.h"

#ifdef __cplusplus
#error "This project uses C only."
#endif

/* =========================================================================
   SECTION: Types
   ========================================================================= */
typedef struct {
    sensor_data_t *data;
    i2c_master_bus_handle_t bus;
} sensor_task_context_t;

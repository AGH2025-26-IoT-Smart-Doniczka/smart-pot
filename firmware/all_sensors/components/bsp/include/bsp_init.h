#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
#error "This project uses C only."
#endif

/* =========================================================================
   SECTION: I2C Helpers (Disposable Buses)
   ========================================================================= */
esp_err_t bsp_i2c_create_display_bus(i2c_master_bus_handle_t *out_bus);
esp_err_t bsp_i2c_create_sensors_bus(i2c_master_bus_handle_t *out_bus);
esp_err_t bsp_i2c_del_bus(i2c_master_bus_handle_t bus);

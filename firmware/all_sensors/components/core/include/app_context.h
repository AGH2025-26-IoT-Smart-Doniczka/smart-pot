#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_master.h"
#include "app_types.h"
#include "ssd1306.h"
#include "soil_sensor.h"

#ifdef __cplusplus
#error "This project uses C only."
#endif

/* =========================================================================
   SECTION: Types
   ========================================================================= */
typedef struct {
    config_t config;             // provisioned configuration
    bool wifi_connected;         // WiFi link state
    bool time_synced;            // SNTP sync status
    uint32_t data_block_seq;     // rolling sensor block number
    sensor_data_t sensor_data;   // latest sensor readout
    i2c_master_bus_handle_t bus_display;  // disposable bus handle for display
    i2c_master_bus_handle_t bus_sensors;  // disposable bus handle for sensors
    ssd1306_handle_t display;             // shared display handle
    soil_sensor_handle_t soil_sensor;     // shared soil sensor handle
} app_context_t;

/* =========================================================================
   SECTION: API
   ========================================================================= */
esp_err_t app_context_init(void);

esp_err_t app_context_set_config(const config_t *cfg);
esp_err_t app_context_get_config(config_t *out);

void app_context_set_wifi_connected(bool connected);
bool app_context_is_wifi_connected(void);

void app_context_set_time_synced(bool synced);
bool app_context_is_time_synced(void);

uint32_t app_context_next_data_block_seq(void);
uint32_t app_context_peek_data_block_seq(void);

esp_err_t app_context_set_sensor_data(const sensor_data_t *data);
esp_err_t app_context_get_sensor_data(sensor_data_t *out);

esp_err_t app_context_set_display_bus(i2c_master_bus_handle_t bus);
esp_err_t app_context_set_sensors_bus(i2c_master_bus_handle_t bus);
i2c_master_bus_handle_t app_context_get_display_bus(void);
i2c_master_bus_handle_t app_context_get_sensors_bus(void);

esp_err_t app_context_set_display_handle(ssd1306_handle_t handle);
ssd1306_handle_t app_context_get_display_handle(void);
ssd1306_handle_t app_context_ensure_display(void);

esp_err_t app_context_set_soil_sensor(soil_sensor_handle_t handle);
soil_sensor_handle_t app_context_get_soil_sensor(void);

const app_context_t *app_context_ref(void);

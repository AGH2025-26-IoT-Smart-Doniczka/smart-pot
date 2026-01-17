#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

#ifdef __cplusplus
#error "This project uses C only."
#endif

/* =========================================================================
   SECTION: Types
   ========================================================================= */
typedef struct soil_sensor *soil_sensor_handle_t;

/* Optional power pin control. Use GPIO_NUM_NC when not used. */
typedef struct {
    adc_unit_t unit;              // ADC unit (ADC_UNIT_1/2)
    adc_channel_t channel;        // ADC channel
    adc_bitwidth_t bitwidth;      // e.g. ADC_BITWIDTH_12
    adc_atten_t atten;            // e.g. ADC_ATTEN_DB_11
    gpio_num_t power_gpio;        // Optional VCC switch; GPIO_NUM_NC to ignore
    bool power_active_high;       // true if high enables power
    uint8_t sample_count;         // Number of averaged samples (>=1)
    uint16_t settle_ms;           // Delay after power-on before reading
} soil_sensor_config_t;

/* =========================================================================
   SECTION: Lifecycle
   ========================================================================= */
esp_err_t soil_sensor_create(const soil_sensor_config_t *cfg, soil_sensor_handle_t *out_handle);
void soil_sensor_destroy(soil_sensor_handle_t handle);

esp_err_t soil_sensor_enable(soil_sensor_handle_t handle);
esp_err_t soil_sensor_disable(soil_sensor_handle_t handle);

/* =========================================================================
   SECTION: Calibration & Measurements
   ========================================================================= */
// Take an averaged raw ADC sample; requires sensor enabled.
esp_err_t soil_sensor_read_raw(soil_sensor_handle_t handle, uint16_t *out_raw);

esp_err_t soil_sensor_calibrate_dry(soil_sensor_handle_t handle, uint16_t raw);
esp_err_t soil_sensor_calibrate_wet(soil_sensor_handle_t handle, uint16_t raw);

esp_err_t soil_sensor_compute_moisture(soil_sensor_handle_t handle, uint16_t raw, uint8_t *out_pct);

esp_err_t soil_sensor_probe(soil_sensor_handle_t handle, uint16_t *out_raw, uint8_t *out_pct);


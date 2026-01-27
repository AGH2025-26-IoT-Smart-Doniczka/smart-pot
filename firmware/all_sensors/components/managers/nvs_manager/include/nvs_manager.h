#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "app_types.h"
#include "app_constants.h"

#ifdef __cplusplus
#error "This project uses C only."
#endif

/* =========================================================================
   SECTION: Types
   ========================================================================= */
typedef struct {
    uint32_t sample_seq;     // Monotonic sample number (ordering source of truth)
    uint32_t timestamp;      // Unix time if available
    sensor_data_t data;      // Sensor payload
} sensor_sample_t;

/* =========================================================================
   SECTION: API
   ========================================================================= */
esp_err_t nvs_manager_init(void);

esp_err_t nvs_manager_save_config(const config_t *cfg);
esp_err_t nvs_manager_load_config(config_t *out_cfg, bool *has_config);
esp_err_t nvs_manager_clear_config(void);

esp_err_t nvs_manager_store_sample(sensor_sample_t *sample_in);
esp_err_t nvs_manager_get_all_samples(sensor_sample_t *buffer, size_t max_items, size_t *out_count);
esp_err_t nvs_manager_clear_samples(void);

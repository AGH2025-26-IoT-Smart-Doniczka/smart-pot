#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_event.h"

#ifdef __cplusplus
#error "This project uses C only."
#endif

/* =========================================================================
   SECTION: API
   ========================================================================= */
esp_err_t watering_manager_init(esp_event_loop_handle_t loop);
esp_err_t watering_manager_start_async(uint16_t duration_s);
esp_err_t watering_manager_start_blocking(uint16_t duration_s);
void watering_manager_deinit(void);
bool watering_manager_is_active(void);
uint32_t watering_manager_get_last_watering_s(void);

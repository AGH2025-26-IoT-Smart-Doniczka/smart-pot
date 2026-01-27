#pragma once

#include "esp_err.h"

#ifdef __cplusplus
#error "This project uses C only."
#endif

/* =========================================================================
   SECTION: API
   ========================================================================= */
esp_err_t wifi_manager_init(void);
esp_err_t wifi_manager_start(void);
void wifi_manager_stop(void);
esp_err_t wifi_manager_request_time_sync(void);

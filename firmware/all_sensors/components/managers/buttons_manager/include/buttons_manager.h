#pragma once

#include "esp_event.h"
#include "esp_err.h"
#include "esp_check.h"
#include "app_events.h"

#ifdef __cplusplus
#error "This project uses C only."
#endif

/* =========================================================================
   SECTION: Public API
   ========================================================================= */

esp_err_t buttons_manager_init(esp_event_loop_handle_t loop);

esp_err_t buttons_manager_enable_deep_sleep_wakeup(void);

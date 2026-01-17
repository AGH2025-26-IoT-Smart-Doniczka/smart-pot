#pragma once

#include "esp_err.h"

#ifdef __cplusplus
#error "This project uses C only."
#endif

/* =========================================================================
   SECTION: Public API
   ========================================================================= */

esp_err_t ble_provisioning_init(void);

esp_err_t ble_provisioning_start(void);

void ble_provisioning_stop(void);

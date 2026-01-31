#pragma once

#include "esp_err.h"
#include "app_types.h"

#ifdef __cplusplus
#error "This project uses C only."
#endif

/* =========================================================================
   SECTION: API
   ========================================================================= */

esp_err_t json_config_parse(const char *json_str_in, config_t *cfg_in_out);

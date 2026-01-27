#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_event.h"
#include "app_states.h"
#include "app_events.h"

#ifdef __cplusplus
#error "This project uses C only."
#endif

/* =========================================================================
   SECTION: Callback Types
   ========================================================================= */
typedef uint8_t exit_mode_t;
#define EXIT_MODE_DEFAULT     ((exit_mode_t)0)
#define EXIT_MODE_INTERRUPTED ((exit_mode_t)1)

typedef struct {
   void (*on_enter)(void);
   void (*on_exit)(exit_mode_t mode);
} state_callbacks_t;

typedef struct {
   state_callbacks_t init;
   state_callbacks_t calib_soil_dry;
   state_callbacks_t calib_soil_wet;
   state_callbacks_t provisioning;
   state_callbacks_t wifi_connect;
   state_callbacks_t sync_time;
   state_callbacks_t sensing;
   state_callbacks_t data_decision;
   state_callbacks_t mqtt_publish;
   state_callbacks_t flash_store;
   state_callbacks_t factory_reset;
   state_callbacks_t idle;
   state_callbacks_t deep_sleep;
} fsm_callbacks_t;

/* =========================================================================
   SECTION: Public API
   ========================================================================= */
esp_err_t fsm_manager_init(const fsm_callbacks_t *callbacks);

esp_err_t fsm_manager_post_event(app_event_id_t event_id,
                                 const void *event_data,
                                 size_t event_data_size,
                                 uint32_t timeout_ms);

app_state_t fsm_manager_get_state(void);

esp_err_t fsm_manager_set_callbacks(const fsm_callbacks_t *callbacks);

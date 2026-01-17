#pragma once

#include "fsm_manager.h"

#ifdef __cplusplus
#error "This project uses C only."
#endif

/* =========================================================================
   SECTION: State Callback Prototypes
   ========================================================================= */
void state_init_on_enter(void);
void state_init_on_exit(exit_mode_t mode);

void state_calib_soil_dry_on_enter(void);
void state_calib_soil_dry_on_exit(exit_mode_t mode);

void state_calib_soil_wet_on_enter(void);
void state_calib_soil_wet_on_exit(exit_mode_t mode);

void state_provisioning_on_enter(void);
void state_provisioning_on_exit(exit_mode_t mode);

void state_wifi_connect_on_enter(void);
void state_wifi_connect_on_exit(exit_mode_t mode);

void state_sync_time_on_enter(void);
void state_sync_time_on_exit(exit_mode_t mode);

void state_sensing_on_enter(void);
void state_sensing_on_exit(exit_mode_t mode);

void state_data_decision_on_enter(void);
void state_data_decision_on_exit(exit_mode_t mode);

void state_mqtt_publish_on_enter(void);
void state_mqtt_publish_on_exit(exit_mode_t mode);

void state_flash_store_on_enter(void);
void state_flash_store_on_exit(exit_mode_t mode);

void state_factory_reset_on_enter(void);
void state_factory_reset_on_exit(exit_mode_t mode);

void state_idle_on_enter(void);
void state_idle_on_exit(exit_mode_t mode);

void state_deep_sleep_on_enter(void);
void state_deep_sleep_on_exit(exit_mode_t mode);

/* =========================================================================
   SECTION: Factories
   ========================================================================= */
const fsm_callbacks_t *fsm_state_callbacks_get(void);

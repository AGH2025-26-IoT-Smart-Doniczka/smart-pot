idf_component_register(
    SRCS
        "src/fsm_manager.c"
        "src/fsm_state_callbacks.c"
        "src/state_init.c"
        "src/state_calib_soil_dry.c"
        "src/state_calib_soil_wet.c"
        "src/state_provisioning.c"
        "src/state_wifi_connect.c"
        "src/state_sync_time.c"
        "src/state_sensing.c"
        "src/state_data_decision.c"
        "src/state_mqtt_publish.c"
        "src/state_flash_store.c"
        "src/state_factory_reset.c"
        "src/state_idle.c"
        "src/state_deep_sleep.c"
    INCLUDE_DIRS "include"
    REQUIRES core esp_event esp_timer bsp display nvs_manager buttons_manager esp_partition ble_provisioning wifi_manager env_sensor mqtt_manager
)

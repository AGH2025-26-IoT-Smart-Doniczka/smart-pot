idf_component_register(
    SRCS "src/ble_provisioning.c"
    INCLUDE_DIRS "include"
    REQUIRES core nvs_manager esp_event bt nvs_flash freertos fsm_manager display
)

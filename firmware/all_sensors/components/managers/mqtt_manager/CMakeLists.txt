idf_component_register(
    SRCS "src/mqtt_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES core esp_event mqtt json driver freertos fsm_manager
)

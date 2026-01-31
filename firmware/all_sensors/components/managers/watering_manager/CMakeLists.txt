idf_component_register(
    SRCS "src/watering_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_timer driver esp_event fsm_manager
)

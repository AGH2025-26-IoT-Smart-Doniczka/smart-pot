idf_component_register(
    SRCS "src/buttons_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_event esp_timer driver esp_pm core bsp button
)

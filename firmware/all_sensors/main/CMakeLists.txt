idf_component_register(
       SRCS "main.c"
       PRIV_REQUIRES spi_flash esp_event core  nvs_manager fsm_manager
    INCLUDE_DIRS ""
)

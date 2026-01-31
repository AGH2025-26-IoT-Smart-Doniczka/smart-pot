idf_component_register(
    SRCS "src/mqtt_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES core esp_event esp_netif mqtt json driver freertos fsm_manager json_parser
)

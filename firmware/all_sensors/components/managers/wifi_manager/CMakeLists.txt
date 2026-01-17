idf_component_register(
    SRCS "src/wifi_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES core esp_event esp_wifi esp_netif nvs_flash freertos fsm_manager lwip
)

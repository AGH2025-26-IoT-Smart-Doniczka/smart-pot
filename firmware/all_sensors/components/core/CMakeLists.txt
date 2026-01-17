idf_component_register(
    SRCS "src/app_context.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_i2c display soil_sensor bsp
)

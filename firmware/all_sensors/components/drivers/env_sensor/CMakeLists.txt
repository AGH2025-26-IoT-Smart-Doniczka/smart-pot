idf_component_register(
    SRCS "bme280.c" "src/bme280_task.c" "src/veml7700.c" "src/veml7700_task.c" "src/soil_sensor_task.c"
    INCLUDE_DIRS "." "include"
    REQUIRES driver freertos esp_timer esp_rom bsp core soil_sensor esp_driver_i2c
)

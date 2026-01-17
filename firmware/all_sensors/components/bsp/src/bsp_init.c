#include "board_pins.h"
#include "bsp_init.h"
#include "esp_log.h"
#include "driver/i2c_master.h"

static const char *TAG = "BSP_INIT";

static esp_err_t create_master_bus(gpio_num_t sda,
                                   gpio_num_t scl,
                                   i2c_port_t port,
                                   i2c_master_bus_handle_t *out_bus)
{
    if (out_bus == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Creating I2C master bus: port=%d sda=%d scl=%d pullups=%d", (int)port, (int)sda, (int)scl, (int)BSP_I2C_PULLUP_ENABLE);

    i2c_master_bus_config_t cfg = {
        .i2c_port = port,
        .sda_io_num = sda,
        .scl_io_num = scl,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .flags.enable_internal_pullup = BSP_I2C_PULLUP_ENABLE,
        .glitch_ignore_cnt = 7
    };

    return i2c_new_master_bus(&cfg, out_bus);
}

esp_err_t bsp_i2c_create_display_bus(i2c_master_bus_handle_t *out_bus)
{
    ESP_LOGI(TAG, "Createing display bus");
    return create_master_bus(BSP_I2C0_SDA_PIN,
                             BSP_I2C0_SCL_PIN,
                             BSP_I2C_NUM_DISPLAY,
                             out_bus);
}

esp_err_t bsp_i2c_create_sensors_bus(i2c_master_bus_handle_t *out_bus)
{
    return create_master_bus(BSP_I2C1_SDA_PIN,
                             BSP_I2C1_SCL_PIN,
                             BSP_I2C_NUM_SENSORS,
                             out_bus);
}

esp_err_t bsp_i2c_del_bus(i2c_master_bus_handle_t bus)
{
    if (bus == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_del_master_bus(bus);
}

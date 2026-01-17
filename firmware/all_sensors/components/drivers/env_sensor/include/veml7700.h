#ifndef VEML7700_H
#define VEML7700_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/i2c_master.h"

// I2C Address
#define VEML7700_I2C_ADDR           0x10

// Command Registers
#define VEML7700_REG_ALS_CONF       0x00
#define VEML7700_REG_ALS_WH         0x01
#define VEML7700_REG_ALS_WL         0x02
#define VEML7700_REG_PSM            0x03
#define VEML7700_REG_ALS            0x04
#define VEML7700_REG_WHITE          0x05
#define VEML7700_REG_ALS_INT        0x06

#define VEML7700_ALS_SM_MASK        0x1800
#define VEML7700_ALS_SM_SHIFT       11
#define VEML7700_ALS_SM_1           (0x00 << 11)
#define VEML7700_ALS_SM_2           (0x01 << 11)
#define VEML7700_ALS_SM_1_8         (0x02 << 11)
#define VEML7700_ALS_SM_1_4         (0x03 << 11)

#define VEML7700_ALS_IT_MASK        0x03C0
#define VEML7700_ALS_IT_SHIFT       6
#define VEML7700_ALS_IT_25MS        (0x0C << 6)
#define VEML7700_ALS_IT_50MS        (0x08 << 6)
#define VEML7700_ALS_IT_100MS       (0x00 << 6)
#define VEML7700_ALS_IT_200MS       (0x01 << 6)
#define VEML7700_ALS_IT_400MS       (0x02 << 6)
#define VEML7700_ALS_IT_800MS       (0x03 << 6)

#define VEML7700_ALS_PERS_MASK      0x0030
#define VEML7700_ALS_PERS_SHIFT     4
#define VEML7700_ALS_PERS_1         (0x00 << 4)
#define VEML7700_ALS_PERS_2         (0x01 << 4)
#define VEML7700_ALS_PERS_4         (0x02 << 4)
#define VEML7700_ALS_PERS_8         (0x03 << 4)

#define VEML7700_ALS_INT_EN_MASK    0x0002
#define VEML7700_ALS_INT_EN_SHIFT   1
#define VEML7700_ALS_INT_DISABLE    (0x00 << 1)
#define VEML7700_ALS_INT_ENABLE     (0x01 << 1)

#define VEML7700_ALS_SD_MASK        0x0001
#define VEML7700_ALS_SD_SHIFT       0
#define VEML7700_ALS_POWER_ON       0x00
#define VEML7700_ALS_SHUTDOWN       0x01

#define VEML7700_PSM_MASK           0x0006
#define VEML7700_PSM_SHIFT          1
#define VEML7700_PSM_MODE_1         (0x00 << 1)
#define VEML7700_PSM_MODE_2         (0x01 << 1)
#define VEML7700_PSM_MODE_3         (0x02 << 1)
#define VEML7700_PSM_MODE_4         (0x03 << 1)

#define VEML7700_PSM_EN_MASK        0x0001
#define VEML7700_PSM_DISABLE        0x00
#define VEML7700_PSM_ENABLE         0x01

#define VEML7700_INT_TH_LOW         0x8000
#define VEML7700_INT_TH_HIGH        0x4000

#define VEML7700_RESOLUTION_BASE    0.0576f
#define VEML7700_LUX_MAX            120000.0f

#define VEML7700_DEFAULT_CONFIG     (VEML7700_ALS_SM_1 | VEML7700_ALS_IT_100MS | \
                                     VEML7700_ALS_PERS_1 | VEML7700_ALS_INT_DISABLE | \
                                     VEML7700_ALS_POWER_ON)

#define VEML7700_INT_CONFIG         (VEML7700_ALS_SM_1 | VEML7700_ALS_IT_100MS | \
                                     VEML7700_ALS_PERS_1 | VEML7700_ALS_INT_ENABLE | \
                                     VEML7700_ALS_POWER_ON)

#define VEML7700_HIGH_SENS_CONFIG   (VEML7700_ALS_SM_2 | VEML7700_ALS_IT_800MS | \
                                     VEML7700_ALS_PERS_1 | VEML7700_ALS_INT_DISABLE | \
                                     VEML7700_ALS_POWER_ON)

#define VEML7700_LOW_POWER_CONFIG   (VEML7700_ALS_SM_1_8 | VEML7700_ALS_IT_25MS | \
                                     VEML7700_ALS_PERS_1 | VEML7700_ALS_INT_DISABLE | \
                                     VEML7700_ALS_POWER_ON)

typedef enum {
    VEML7700_LIGHT_WARM,
    VEML7700_LIGHT_NEUTRAL,
    VEML7700_LIGHT_COLD,
    VEML7700_LIGHT_UNKNOWN
} veml7700_color_temp_t;

esp_err_t veml7700_write_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint16_t value);
esp_err_t veml7700_read_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint16_t *value);
void veml7700_print_config(uint16_t config);
uint16_t veml7700_lux_to_raw(float lux, uint16_t config);

esp_err_t veml7700_init(i2c_master_dev_handle_t dev_handle, uint16_t config);
esp_err_t veml7700_read_config(i2c_master_dev_handle_t dev_handle, uint16_t *config);

esp_err_t veml7700_read_als(i2c_master_dev_handle_t dev_handle, uint16_t *als_data);
esp_err_t veml7700_read_white(i2c_master_dev_handle_t dev_handle, uint16_t *white_data);
esp_err_t veml7700_read_lux(i2c_master_dev_handle_t dev_handle, float *lux);
esp_err_t veml7700_shutdown(i2c_master_dev_handle_t dev_handle);
esp_err_t veml7700_power_on(i2c_master_dev_handle_t dev_handle);
esp_err_t veml7700_set_threshold(i2c_master_dev_handle_t dev_handle, uint16_t low, uint16_t high);
esp_err_t veml7700_get_interrupt_status(i2c_master_dev_handle_t dev_handle, uint16_t *status);
esp_err_t veml7700_enable_interrupt(i2c_master_dev_handle_t dev_handle, bool enable);

float veml7700_raw_to_lux(uint16_t raw_als, uint16_t config);
float veml7700_get_resolution(uint16_t config);
veml7700_color_temp_t veml7700_get_color_temp(uint16_t als, uint16_t white);
float veml7700_get_als_white_ratio(uint16_t als, uint16_t white);

#endif // VEML7700_H

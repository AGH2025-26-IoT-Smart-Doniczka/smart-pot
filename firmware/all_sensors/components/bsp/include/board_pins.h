#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_adc/adc_oneshot.h"

/* =========================================================================
	SECTION: I2C Buses (Display / Sensors)
	========================================================================= */
#define BSP_I2C_NUM_DISPLAY     I2C_NUM_0
#define BSP_I2C_NUM_SENSORS     I2C_NUM_1

#define BSP_I2C0_SDA_PIN        GPIO_NUM_32
#define BSP_I2C0_SCL_PIN        GPIO_NUM_33

#define BSP_I2C1_SDA_PIN        GPIO_NUM_25
#define BSP_I2C1_SCL_PIN        GPIO_NUM_26

#define BSP_I2C_PULLUP_ENABLE   true

/* =========================================================================
	SECTION: ADC (Soil Moisture)
	========================================================================= */
#define BSP_ADC_SOIL_PIN        GPIO_NUM_34
#define BSP_ADC_UNIT            ADC_UNIT_1
#define BSP_ADC_CHANNEL         ADC_CHANNEL_6
#define BSP_ADC_ATTEN           ADC_ATTEN_DB_12
#define BSP_ADC_WIDTH           ADC_BITWIDTH_9

/* =========================================================================
	SECTION: Buttons
	========================================================================= */
#define BSP_BTN1_PIN            GPIO_NUM_14
#define BSP_BTN1_ACTIVE_LEVEL   0

#define BSP_BTN2_PIN            GPIO_NUM_27
#define BSP_BTN2_ACTIVE_LEVEL   0

#endif // BOARD_PINS_H

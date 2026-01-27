#ifndef APP_TYPES_H
#define APP_TYPES_H

#include <stdint.h>

// Plant configuration structure
typedef struct {
    uint8_t lux;              // Light level: 0=dużo, 1=średnio, 2=mało
    uint8_t moi[4];           // Soil moisture thresholds (0-100)
    uint16_t tem[2];          // Temperature thresholds in Kelvins*10
    uint16_t sleep_duration;  // Sleep duration in seconds
} plant_config_t;

/* =========================================================================
   SECTION: Sensor Data
   ========================================================================= */
typedef struct {
    uint32_t timestamp;     // unix timestamp
    uint16_t lux_level;     // 0/1/2
    uint8_t soil_moisture;  // ADC value or %
    uint16_t temperature;   // deci-Kelvin
    float pressure;         // hPa
} sensor_data_t;

// Configuration structure
typedef struct {
    char ssid[32];
    char passwd[64];
    plant_config_t plant_config;
    char mqtt_passwd[32];
    uint16_t sleep_duration;
    uint16_t soil_adc_dry;   // ADC_BITWIDTH_9 
    uint16_t soil_adc_wet;   // ADC_BITWIDTH_9
} config_t;

#endif // APP_TYPES_H

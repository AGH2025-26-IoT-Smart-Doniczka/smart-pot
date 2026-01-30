#ifndef APP_TYPES_H
#define APP_TYPES_H

#include <stdint.h>

// Plant configuration structure
typedef struct {
    uint8_t moi[2];           // Soil moisture thresholds 
    uint16_t tem[2];          // Temperature thresholds in Kelvins*10: min/max
} plant_config_t;

/* =========================================================================
   SECTION: Sensor Data
   ========================================================================= */
typedef struct {
    // uint32_t timestamp;     // unix timestamp
    uint16_t lux_level;     // lux
    uint8_t soil_moisture;  
    uint16_t temperature;   // deci-Kelvin
    float pressure;         // hPa
} sensor_data_t;

// Configuration structure
typedef struct {
    char ssid[32];
    char passwd[64];
    plant_config_t plant_config;
    char mqtt_passwd[32];
    uint16_t mes;             // Measurement interval in seconds
    uint16_t sen;             // Sending interval in seconds
    uint16_t wat;             // Watering duration in seconds
    uint16_t wai;             // Watering interval in seconds (0=disabled)
    uint16_t soil_adc_dry;   // ADC_BITWIDTH_9 
    uint16_t soil_adc_wet;   // ADC_BITWIDTH_9
} config_t;

#endif // APP_TYPES_H

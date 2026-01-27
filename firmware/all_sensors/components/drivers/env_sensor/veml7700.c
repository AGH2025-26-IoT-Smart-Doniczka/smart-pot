#include "veml7700.h"
#include "esp_log.h"

static const char *TAG = "VEML7700";

// Resolution lookup table: [gain_index][it_index]
// Gain index: 0=x1, 1=x2, 2=x1/8, 3=x1/4
// IT index: 0=25ms, 1=50ms, 2=100ms, 3=200ms, 4=400ms, 5=800ms
static const float resolution_table[4][6] = {
    // Gain x1
    {1.8432f, 0.9216f, 0.4608f, 0.2304f, 0.1152f, 0.0576f},
    // Gain x2
    {0.9216f, 0.4608f, 0.2304f, 0.1152f, 0.0576f, 0.0288f},
    // Gain x1/8
    {14.7456f, 7.3728f, 3.6864f, 1.8432f, 0.9216f, 0.4608f},
    // Gain x1/4
    {7.3728f, 3.6864f, 1.8432f, 0.9216f, 0.4608f, 0.2304f},
};

static int get_gain_index(uint16_t config) {
    uint8_t sm = (config & VEML7700_ALS_SM_MASK) >> VEML7700_ALS_SM_SHIFT;
    return sm;  // 0=x1, 1=x2, 2=x1/8, 3=x1/4
}

static int get_it_index(uint16_t config) {
    uint8_t it = (config & VEML7700_ALS_IT_MASK) >> VEML7700_ALS_IT_SHIFT;
    switch (it) {
        case 0x0C: return 0;  // 25ms
        case 0x08: return 1;  // 50ms
        case 0x00: return 2;  // 100ms
        case 0x01: return 3;  // 200ms
        case 0x02: return 4;  // 400ms
        case 0x03: return 5;  // 800ms
        default:   return 2;  // default to 100ms
    }
}

float veml7700_raw_to_lux(uint16_t raw_als, uint16_t config) {
    int gain_idx = get_gain_index(config);
    int it_idx = get_it_index(config);
    
    float resolution = resolution_table[gain_idx][it_idx];
    float lux = (float)raw_als * resolution;
    
    if (lux > 1000.0f) {
            lux = (((6.0135e-13f * lux - 9.3924e-9f) * lux + 8.1488e-5f) * lux + 1.0023f) * lux;
    }
    
    return lux;
}

float veml7700_get_resolution(uint16_t config) {
    int gain_idx = get_gain_index(config);
    int it_idx = get_it_index(config);
    return resolution_table[gain_idx][it_idx];
}

esp_err_t veml7700_write_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint16_t value) {
    uint8_t data[3] = {reg, value & 0xFF, (value >> 8) & 0xFF};
    return i2c_master_transmit(dev_handle, data, 3, 100);
}

esp_err_t veml7700_read_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint16_t *value) {
    uint8_t data[2];
    esp_err_t ret = i2c_master_transmit_receive(dev_handle, &reg, 1, data, 2, 100);
    if (ret == ESP_OK) {
        *value = data[0] | (data[1] << 8);
    }
    return ret;
}

esp_err_t veml7700_init(i2c_master_dev_handle_t dev_handle, uint16_t config) {
    esp_err_t ret = veml7700_write_reg(dev_handle, VEML7700_REG_ALS_CONF, config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "VEML7700 initialized with config 0x%04X", config);
    }
    return ret;
}

esp_err_t veml7700_read_config(i2c_master_dev_handle_t dev_handle, uint16_t *config) {
    return veml7700_read_reg(dev_handle, VEML7700_REG_ALS_CONF, config);
}

void veml7700_print_config(uint16_t config) {

    uint8_t gain = (config & VEML7700_ALS_SM_MASK) >> VEML7700_ALS_SM_SHIFT;
    const char *gain_str;
    switch (gain) {
        case 0: gain_str = "x1"; break;
        case 1: gain_str = "x2"; break;
        case 2: gain_str = "x1/8"; break;
        case 3: gain_str = "x1/4"; break;
        default: gain_str = "unknown"; break;
    }
    
    uint8_t it = (config & VEML7700_ALS_IT_MASK) >> VEML7700_ALS_IT_SHIFT;
    const char *it_str;
    switch (it) {
        case 0x0C: it_str = "25ms"; break;
        case 0x08: it_str = "50ms"; break;
        case 0x00: it_str = "100ms"; break;
        case 0x01: it_str = "200ms"; break;
        case 0x02: it_str = "400ms"; break;
        case 0x03: it_str = "800ms"; break;
        default: it_str = "unknown"; break;
    }

    uint8_t pers = (config & VEML7700_ALS_PERS_MASK) >> VEML7700_ALS_PERS_SHIFT;
    
    bool int_en = (config & VEML7700_ALS_INT_EN_MASK) != 0;
    
    bool shutdown = (config & VEML7700_ALS_SD_MASK) != 0;
    
    ESP_LOGI(TAG, "VEML7700 Config (0x%04X):", config);
    ESP_LOGI(TAG, "  Gain: %s", gain_str);
    ESP_LOGI(TAG, "  Integration Time: %s", it_str);
    ESP_LOGI(TAG, "  Persistence: %u samples", 1 << pers);
    ESP_LOGI(TAG, "  Interrupt: %s", int_en ? "enabled" : "disabled");
    ESP_LOGI(TAG, "  Power: %s", shutdown ? "shutdown" : "on");
}

esp_err_t veml7700_read_als(i2c_master_dev_handle_t dev_handle, uint16_t *als_data) {
    return veml7700_read_reg(dev_handle, VEML7700_REG_ALS, als_data);
}

esp_err_t veml7700_read_white(i2c_master_dev_handle_t dev_handle, uint16_t *white_data) {
    return veml7700_read_reg(dev_handle, VEML7700_REG_WHITE, white_data);
}

esp_err_t veml7700_read_lux(i2c_master_dev_handle_t dev_handle, float *lux) {
    uint16_t als_raw, config;
    
    esp_err_t ret = veml7700_read_config(dev_handle, &config);
    if (ret != ESP_OK) return ret;
    
    ret = veml7700_read_als(dev_handle, &als_raw);
    if (ret != ESP_OK) return ret;
    
    *lux = veml7700_raw_to_lux(als_raw, config);
    return ESP_OK;
}

esp_err_t veml7700_shutdown(i2c_master_dev_handle_t dev_handle) {
    uint16_t config;
    esp_err_t ret = veml7700_read_config(dev_handle, &config);
    if (ret != ESP_OK) return ret;
    
    config |= VEML7700_ALS_SHUTDOWN;
    return veml7700_write_reg(dev_handle, VEML7700_REG_ALS_CONF, config);
}

esp_err_t veml7700_power_on(i2c_master_dev_handle_t dev_handle) {
    uint16_t config;
    esp_err_t ret = veml7700_read_config(dev_handle, &config);
    if (ret != ESP_OK) return ret;
    
    config &= ~VEML7700_ALS_SD_MASK;
    return veml7700_write_reg(dev_handle, VEML7700_REG_ALS_CONF, config);
}

esp_err_t veml7700_set_threshold(i2c_master_dev_handle_t dev_handle, uint16_t low, uint16_t high) {
    esp_err_t ret = veml7700_write_reg(dev_handle, VEML7700_REG_ALS_WL, low);
    if (ret != ESP_OK) return ret;
    return veml7700_write_reg(dev_handle, VEML7700_REG_ALS_WH, high);
}

esp_err_t veml7700_get_interrupt_status(i2c_master_dev_handle_t dev_handle, uint16_t *status) {
    return veml7700_read_reg(dev_handle, VEML7700_REG_ALS_INT, status);
}

uint16_t veml7700_lux_to_raw(float lux, uint16_t config) {
    int gain_idx = get_gain_index(config);
    int it_idx = get_it_index(config);
    float resolution = resolution_table[gain_idx][it_idx];
    
    uint32_t raw = (uint32_t)(lux / resolution);
    if (raw > 65535) raw = 65535;
    return (uint16_t)raw;
}

veml7700_color_temp_t veml7700_get_color_temp(uint16_t als, uint16_t white) {
    if (white == 0) return VEML7700_LIGHT_UNKNOWN;
    
    float ratio = (float)als / (float)white;
    
    if (ratio > 0.9f) {
        return VEML7700_LIGHT_COLD;
    } else if (ratio < 0.5f) {
        return VEML7700_LIGHT_WARM;
    } else {
        return VEML7700_LIGHT_NEUTRAL;
    }
}

float veml7700_get_als_white_ratio(uint16_t als, uint16_t white) {
    if (white == 0) return 0.0f;
    return (float)als / (float)white;
}

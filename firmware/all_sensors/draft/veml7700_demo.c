#include <stdio.h>
#include <string.h>
#include <stdatomic.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "ssd1306.h"
#include "i2c.h"
#include "veml7700.h"

static const char *TAG = "VEML7700_DEMO";

#define LUX_LOW_THRESHOLD   300.0f
#define LUX_HIGH_THRESHOLD  100000.0f

static QueueHandle_t sensor_queue = NULL;
static atomic_bool threshold_low_triggered = false;
static atomic_bool threshold_high_triggered = false;

typedef struct {
    float lux;
    uint16_t raw_als;
    uint16_t white;
} sensor_data_t;

static i2c_master_dev_handle_t oled_handle = NULL;
static i2c_master_dev_handle_t veml_handle = NULL;
static TaskHandle_t sensor_task_handle = NULL;
static TaskHandle_t display_task_handle = NULL;

static void sensor_read_task(void *pvParameters) {
    sensor_data_t data;
    uint16_t config;
    uint16_t int_status;


    uint16_t counter = 0;
    
    vTaskDelay(pdMS_TO_TICKS(100));
    
    while (1) {
        esp_err_t ret = veml7700_read_als(veml_handle, &data.raw_als);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read ALS: %s", esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        
        ret = veml7700_read_white(veml_handle, &data.white);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read white: %s", esp_err_to_name(ret));
        }
        
        ret = veml7700_read_config(veml_handle, &config);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read config: %s", esp_err_to_name(ret));
        } else {
            veml7700_print_config(config);
        }

        ret = veml7700_get_interrupt_status(veml_handle, &int_status);
        if (ret == ESP_OK) {
            if (int_status & VEML7700_INT_TH_LOW) {
                atomic_store(&threshold_low_triggered, true);
                ESP_LOGW(TAG, "Low threshold interrupt triggered!");
            } else {
                atomic_store(&threshold_low_triggered, false);
            }
            if (int_status & VEML7700_INT_TH_HIGH) {
                atomic_store(&threshold_high_triggered, true);
                ESP_LOGW(TAG, "High threshold interrupt triggered!");
            } else {
                atomic_store(&threshold_high_triggered, false);
            }
        }        
        
        data.lux = veml7700_raw_to_lux(data.raw_als, config);
        
        ESP_LOGI(TAG, "Lux: %.2f, Raw: %u, White: %u", data.lux, data.raw_als, data.white);
        
        xQueueOverwrite(sensor_queue, &data);
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static const char* color_temp_to_str(veml7700_color_temp_t temp) {
    switch (temp) {
        case VEML7700_LIGHT_WARM:    return "Warm";
        case VEML7700_LIGHT_NEUTRAL: return "Neutral";
        case VEML7700_LIGHT_COLD:    return "Cold";
        default:                     return "Unknown";
    }
}

static void display_task(void *pvParameters) {
    sensor_data_t data;
    char line1[32];
    char line2[32];
    char line3[32];
    char line4[32];
    
    while (1) {
        if (xQueueReceive(sensor_queue, &data, portMAX_DELAY) == pdTRUE) {
            float ratio = veml7700_get_als_white_ratio(data.raw_als, data.white);
            veml7700_color_temp_t temp = veml7700_get_color_temp(data.raw_als, data.white);
            
            snprintf(line1, sizeof(line1), "ALS:%u W:%u", data.raw_als, data.white);

            snprintf(line2, sizeof(line2), "Ratio: %.3f", ratio);
            snprintf(line3, sizeof(line3), "Light: %s", color_temp_to_str(temp));
            snprintf(line4, sizeof(line4), "Lux: %.1f", data.lux);
            
            ssd1306_clear_buffer();
            ssd1306_draw_text(line1, 0, 0);
            ssd1306_draw_text(line2, 0, 2);
            ssd1306_draw_text(line3, 0, 4);
            ssd1306_draw_text(line4, 0, 6);
            
            if (atomic_load(&threshold_low_triggered)) {
                ssd1306_fill_rect(118, 56, 4, 6, true);  
            }
            if (atomic_load(&threshold_high_triggered)) {
                ssd1306_fill_rect(124, 56, 4, 6, true);  
            }
            
            ssd1306_update_display(oled_handle);
        }
    }
}

static i2c_master_dev_handle_t veml7700_init_device(i2c_master_bus_handle_t bus_handle) {
    i2c_master_dev_handle_t dev_handle;
    
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = VEML7700_I2C_ADDR,
        .scl_speed_hz = 100000,
    };
    
    esp_err_t ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add VEML7700 device: %s", esp_err_to_name(ret));
        return NULL;
    }
    
    return dev_handle;
}

void veml7700_demo_start(i2c_master_bus_handle_t bus_handle, i2c_master_dev_handle_t oled) {
    oled_handle = oled;
    atomic_store(&threshold_low_triggered, false);
    atomic_store(&threshold_high_triggered, false);
    
    ESP_LOGI(TAG, "Initializing VEML7700...");
    veml_handle = veml7700_init_device(bus_handle);
    if (veml_handle == NULL) {
        ESP_LOGE(TAG, "Failed to initialize VEML7700");
        return;
    }
    
    uint16_t raw_low = veml7700_lux_to_raw(LUX_LOW_THRESHOLD, VEML7700_INT_CONFIG);
    uint16_t raw_high = veml7700_lux_to_raw(LUX_HIGH_THRESHOLD, VEML7700_INT_CONFIG);
    ESP_LOGI(TAG, "Setting thresholds: low=%u raw (%.0f lux), high=%u raw (%.0f lux)", 
             raw_low, LUX_LOW_THRESHOLD, raw_high, LUX_HIGH_THRESHOLD);
    
    esp_err_t ret = veml7700_set_threshold(veml_handle, raw_low, raw_high);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set thresholds: %s", esp_err_to_name(ret));
    }
    
    ret = veml7700_init(veml_handle, VEML7700_INT_CONFIG);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure VEML7700: %s", esp_err_to_name(ret));
        return;
    }
    
    sensor_queue = xQueueCreate(1, sizeof(sensor_data_t));
    if (sensor_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create queue");
        return;
    }
    
    xTaskCreate(sensor_read_task, "sensor_read", 4096, NULL, 5, &sensor_task_handle);
    xTaskCreate(display_task, "display", 4096, NULL, 4, &display_task_handle);
    
    ESP_LOGI(TAG, "VEML7700 demo started");
}

void veml7700_demo_stop(void) {
    if (sensor_task_handle != NULL) {
        vTaskDelete(sensor_task_handle);
        sensor_task_handle = NULL;
    }
    if (display_task_handle != NULL) {
        vTaskDelete(display_task_handle);
        display_task_handle = NULL;
    }
    if (sensor_queue != NULL) {
        vQueueDelete(sensor_queue);
        sensor_queue = NULL;
    }
    ESP_LOGI(TAG, "VEML7700 demo stopped");
}

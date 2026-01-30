#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "ssd1306.h"
#include "ssd1306_images.h"
#include "bme280_task.h"
#include "veml7700_task.h"
#include "soil_sensor_task.h"
#include "bsp_init.h"
#include "app_context.h"
#include "sensor_task_context.h"
#include "fsm_manager.h"
#include "fsm_state_callbacks.h"

/* =========================================================================
   SECTION: Helpers
   ========================================================================= */
static bool threshold_u8_invalid(uint8_t min, uint8_t max)
{
    return (max <= min);
}

static bool threshold_u16_invalid(uint16_t min, uint16_t max)
{
    return (max <= min);
}

static void display_sensor_data(const sensor_data_t *data)
{
    if (data == NULL) {
        return;
    }

    config_t cfg = {0};
    bool has_cfg = (app_context_get_config(&cfg) == ESP_OK);

    bool stats_off = false;
    if (has_cfg) {
        if (!threshold_u8_invalid(cfg.plant_config.moi[0], cfg.plant_config.moi[1])) {
            if (data->soil_moisture < cfg.plant_config.moi[0] || data->soil_moisture > cfg.plant_config.moi[1]) {
                stats_off = true;
            }
        }
        if (!threshold_u16_invalid(cfg.plant_config.tem[0], cfg.plant_config.tem[1])) {
            if (data->temperature < cfg.plant_config.tem[0] || data->temperature > cfg.plant_config.tem[1]) {
                stats_off = true;
            }
        }
    }

    ssd1306_handle_t disp = app_context_ensure_display();
    if (disp == NULL) {
        return;
    }

    char line1[24] = {0};
    char line2[24] = {0};
    char line3[24] = {0};

    float temp_c = ((float)data->temperature / 10.0f) - 273.15f;
    (void)snprintf(line1, sizeof(line1), "%.1f°C", temp_c);
    (void)snprintf(line2, sizeof(line2), "%u", (unsigned)data->lux_level);
    (void)snprintf(line3, sizeof(line3), "%u%%", (unsigned)data->soil_moisture);

    (void)ssd1306_clear(disp);
    (void)ssd1306_draw_bitmap(disp, &SSD1306_IMAGE_HEADER);
    (void)ssd1306_draw_bitmap(disp, &SSD1306_IMAGE_FACE);
    if (stats_off) {
        (void)ssd1306_draw_bitmap(disp, &SSD1306_IMAGE_SAD);
    } else {
        (void)ssd1306_draw_bitmap(disp, &SSD1306_IMAGE_HAPPY);
    }

    if (app_context_is_wifi_connected()) {
        (void)ssd1306_draw_bitmap(disp, &SSD1306_IMAGE_WIFI_OK);
    } else {
        (void)ssd1306_draw_bitmap(disp, &SSD1306_IMAGE_WIFI_NOT_OK);
    }

    (void)ssd1306_draw_bitmap(disp, &SSD1306_IMAGE_THERMOMETER);
    (void)ssd1306_draw_bitmap(disp, &SSD1306_IMAGE_SUN);
    (void)ssd1306_draw_bitmap(disp, &SSD1306_IMAGE_RAINDROP);

    (void)ssd1306_draw_text(disp, line1, 68, 3);
    (void)ssd1306_draw_text(disp, line2, 68, 5);
    (void)ssd1306_draw_text(disp, line3, 68, 7);
    (void)ssd1306_flush(disp);
}

static const char *TAG = "STATE_SENSING";

/* =========================================================================
   SECTION: Callbacks
   ========================================================================= */
void state_sensing_on_enter(void)
{
    ESP_LOGI(TAG, "enter");

    i2c_master_bus_handle_t bus = app_context_get_sensors_bus();
    if (bus == NULL) {
        if (bsp_i2c_create_sensors_bus(&bus) != ESP_OK) {
            ESP_LOGW(TAG, "sensors bus create failed");
            (void)fsm_manager_post_event(APP_EVENT_SENSORS_DATA_READY, NULL, 0, 0);
            return;
        }
        (void)app_context_set_sensors_bus(bus);
        ESP_LOGI(TAG, "sensors bus created");
    }

    sensor_data_t data = {0};
    sensor_task_context_t shared = {
        .data = &data,
        .bus = bus,
    };

    ESP_LOGI(TAG, "start bme280 sync");
    if (bme280_read_once(&shared) != ESP_OK) {
        ESP_LOGW(TAG, "bme280 sync failed");
    }

    ESP_LOGI(TAG, "start veml7700 sync");
    if (veml7700_read_once(&shared) != ESP_OK) {
        ESP_LOGW(TAG, "veml7700 sync failed");
    }

    ESP_LOGI(TAG, "start soil sync");
    if (soil_sensor_read_once(&shared) != ESP_OK) {
        ESP_LOGW(TAG, "soil sync failed");
    }

    (void)app_context_set_sensor_data(&data);
    if (app_context_display_on_wakeup()) {
        display_sensor_data(&data);
    }
    (void)fsm_manager_post_event(APP_EVENT_SENSORS_DATA_READY, NULL, 0, 0);
}

void state_sensing_on_exit(exit_mode_t mode)
{
    (void)mode;
    ESP_LOGI(TAG, "exit");
    // display_shutdown();
}

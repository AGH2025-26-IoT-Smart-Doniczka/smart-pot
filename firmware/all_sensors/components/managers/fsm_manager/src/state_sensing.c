#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "ssd1306.h"
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
static void display_shutdown(void)
{
    ssd1306_handle_t disp = app_context_get_display_handle();
    i2c_master_bus_handle_t bus = app_context_get_display_bus();

    if (disp) {
        (void)ssd1306_power_off(disp);
        ssd1306_destroy(disp);
        (void)app_context_set_display_handle(NULL);
    }
    if (bus) {
        (void)bsp_i2c_del_bus(bus);
        (void)app_context_set_display_bus(NULL);
    }
}

static void display_sensor_data(const sensor_data_t *data)
{
    if (data == NULL) {
        return;
    }

    ssd1306_handle_t disp = app_context_ensure_display();
    if (disp == NULL) {
        return;
    }

    char line1[24] = {0};
    char line2[24] = {0};
    char line3[24] = {0};

    float temp_c = ((float)data->temperature / 10.0f) - 273.15f;
    (void)snprintf(line1, sizeof(line1), "Lux:%u Soil:%u%%",
                   (unsigned)data->lux_level,
                   (unsigned)data->soil_moisture);
    (void)snprintf(line2, sizeof(line2), "Temp:%.1fC", temp_c);
    (void)snprintf(line3, sizeof(line3), "Pres:%.1fhPa", data->pressure);

    (void)ssd1306_clear(disp);
    (void)ssd1306_draw_text(disp, line1, 0, 0);
    (void)ssd1306_draw_text(disp, line2, 0, 2);
    (void)ssd1306_draw_text(disp, line3, 0, 4);
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
    (void)app_context_get_sensor_data(&data);
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
    display_sensor_data(&data);
    (void)fsm_manager_post_event(APP_EVENT_SENSORS_DATA_READY, NULL, 0, 0);
}

void state_sensing_on_exit(exit_mode_t mode)
{
    (void)mode;
    ESP_LOGI(TAG, "exit");
    display_shutdown();
}

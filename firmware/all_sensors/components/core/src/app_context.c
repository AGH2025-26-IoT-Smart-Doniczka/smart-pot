#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "bsp_init.h"
#include "app_context.h"

static const char *TAG = "APP_CTX";

/* =========================================================================
   SECTION: Static State
   ========================================================================= */
static app_context_t s_ctx;
static SemaphoreHandle_t s_ctx_mutex;
static bool s_initialized;

/* =========================================================================
   SECTION: Helpers
   ========================================================================= */
static bool lock_ctx(TickType_t ticks)
{
    if (s_ctx_mutex == NULL) {
        return false;
    }
    return xSemaphoreTake(s_ctx_mutex, ticks) == pdTRUE;
}

static void unlock_ctx(void)
{
    if (s_ctx_mutex != NULL) {
        (void)xSemaphoreGive(s_ctx_mutex);
    }
}

/* =========================================================================
   SECTION: Public API
   ========================================================================= */
esp_err_t app_context_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    memset(&s_ctx, 0, sizeof(s_ctx));
    s_ctx_mutex = xSemaphoreCreateMutex();
    if (s_ctx_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }

    s_initialized = true;
    return ESP_OK;
}

esp_err_t app_context_set_config(const config_t *cfg)
{
    if (!s_initialized || cfg == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!lock_ctx(pdMS_TO_TICKS(50))) {
        return ESP_ERR_TIMEOUT;
    }

    s_ctx.config = *cfg;
    unlock_ctx();
    return ESP_OK;
}

esp_err_t app_context_get_config(config_t *out)
{
    if (!s_initialized || out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!lock_ctx(pdMS_TO_TICKS(50))) {
        return ESP_ERR_TIMEOUT;
    }

    *out = s_ctx.config;
    unlock_ctx();
    return ESP_OK;
}

void app_context_set_wifi_connected(bool connected)
{
    if (!s_initialized || !lock_ctx(pdMS_TO_TICKS(20))) {
        return;
    }
    s_ctx.wifi_connected = connected;
    unlock_ctx();
}

bool app_context_is_wifi_connected(void)
{
    bool connected = false;
    if (!s_initialized) {
        return false;
    }
    if (lock_ctx(pdMS_TO_TICKS(20))) {
        connected = s_ctx.wifi_connected;
        unlock_ctx();
    }
    return connected;
}

void app_context_set_time_synced(bool synced)
{
    if (!s_initialized || !lock_ctx(pdMS_TO_TICKS(20))) {
        return;
    }
    s_ctx.time_synced = synced;
    unlock_ctx();
}

bool app_context_is_time_synced(void)
{
    bool synced = false;
    if (!s_initialized) {
        return false;
    }
    if (lock_ctx(pdMS_TO_TICKS(20))) {
        synced = s_ctx.time_synced;
        unlock_ctx();
    }
    return synced;
}

uint32_t app_context_next_data_block_seq(void)
{
    uint32_t val = 0;
    if (!s_initialized) {
        return 0;
    }
    if (lock_ctx(pdMS_TO_TICKS(20))) {
        s_ctx.data_block_seq++;
        val = s_ctx.data_block_seq;
        unlock_ctx();
    }
    return val;
}

uint32_t app_context_peek_data_block_seq(void)
{
    uint32_t val = 0;
    if (!s_initialized) {
        return 0;
    }
    if (lock_ctx(pdMS_TO_TICKS(20))) {
        val = s_ctx.data_block_seq;
        unlock_ctx();
    }
    return val;
}

esp_err_t app_context_set_sensor_data(const sensor_data_t *data)
{
    if (!s_initialized || data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!lock_ctx(pdMS_TO_TICKS(50))) {
        return ESP_ERR_TIMEOUT;
    }

    s_ctx.sensor_data = *data;
    unlock_ctx();
    return ESP_OK;
}

esp_err_t app_context_get_sensor_data(sensor_data_t *out)
{
    if (!s_initialized || out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!lock_ctx(pdMS_TO_TICKS(50))) {
        return ESP_ERR_TIMEOUT;
    }

    *out = s_ctx.sensor_data;
    unlock_ctx();
    return ESP_OK;
}

esp_err_t app_context_set_display_bus(i2c_master_bus_handle_t bus)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!lock_ctx(pdMS_TO_TICKS(20))) {
        return ESP_ERR_TIMEOUT;
    }
    s_ctx.bus_display = bus;
    unlock_ctx();
    return ESP_OK;
}

esp_err_t app_context_set_sensors_bus(i2c_master_bus_handle_t bus)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!lock_ctx(pdMS_TO_TICKS(20))) {
        return ESP_ERR_TIMEOUT;
    }
    s_ctx.bus_sensors = bus;
    unlock_ctx();
    return ESP_OK;
}

i2c_master_bus_handle_t app_context_get_display_bus(void)
{
    i2c_master_bus_handle_t h = NULL;
    if (!s_initialized) {
        return NULL;
    }
    if (lock_ctx(pdMS_TO_TICKS(20))) {
        h = s_ctx.bus_display;
        unlock_ctx();
    }
    return h;
}

esp_err_t app_context_set_display_handle(ssd1306_handle_t handle)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!lock_ctx(pdMS_TO_TICKS(20))) {
        return ESP_ERR_TIMEOUT;
    }
    s_ctx.display = handle;
    unlock_ctx();
    return ESP_OK;
}

ssd1306_handle_t app_context_get_display_handle(void)
{
    ssd1306_handle_t h = NULL;
    if (!s_initialized) {
        return NULL;
    }
    if (lock_ctx(pdMS_TO_TICKS(20))) {
        h = s_ctx.display;
        unlock_ctx();
    }
    return h;
}

i2c_master_bus_handle_t app_context_get_sensors_bus(void)
{
    i2c_master_bus_handle_t h = NULL;
    if (!s_initialized) {
        return NULL;
    }
    if (lock_ctx(pdMS_TO_TICKS(20))) {
        h = s_ctx.bus_sensors;
        unlock_ctx();
    }
    return h;
}

const app_context_t *app_context_ref(void)
{
    return s_initialized ? &s_ctx : NULL;
}

esp_err_t app_context_set_soil_sensor(soil_sensor_handle_t handle)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!lock_ctx(pdMS_TO_TICKS(20))) {
        return ESP_ERR_TIMEOUT;
    }
    s_ctx.soil_sensor = handle;
    unlock_ctx();
    return ESP_OK;
}

soil_sensor_handle_t app_context_get_soil_sensor(void)
{
    soil_sensor_handle_t h = NULL;
    if (!s_initialized) {
        return NULL;
    }
    if (lock_ctx(pdMS_TO_TICKS(20))) {
        h = s_ctx.soil_sensor;
        unlock_ctx();
    }
    return h;
}

ssd1306_handle_t app_context_ensure_display(void)
{
    i2c_master_bus_handle_t bus = app_context_get_display_bus();
    if (bus == NULL) {
        if (bsp_i2c_create_display_bus(&bus) != ESP_OK) {
            ESP_LOGW(TAG, "display bus create failed");
            return NULL;
        }
        (void)app_context_set_display_bus(bus);
    }

    ssd1306_handle_t disp = app_context_get_display_handle();
    if (disp == NULL) {
        if (ssd1306_create(bus, &disp) != ESP_OK) {
            ESP_LOGW(TAG, "display create failed");
            return NULL;
        }
        (void)app_context_set_display_handle(disp);
    }
    return disp;
}

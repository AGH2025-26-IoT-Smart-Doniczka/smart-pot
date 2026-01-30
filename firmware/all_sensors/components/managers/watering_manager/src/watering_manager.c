#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "app_events.h"
#include "watering_manager.h"

/* =========================================================================
   SECTION: Constants
   ========================================================================= */
#define WATER_GPIO_NUM GPIO_NUM_2
#define WATER_MAX_DURATION_S 60U

/* =========================================================================
   SECTION: Static Data
   ========================================================================= */
static const char *TAG = "WATER_MGR";
static esp_timer_handle_t s_timer;
static bool s_active;
static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;
static esp_event_loop_handle_t s_loop;
static bool s_gpio_initialized;
RTC_DATA_ATTR static uint32_t s_last_watering_s = 0;

/* =========================================================================
   SECTION: Helpers
   ========================================================================= */
static uint32_t clamp_duration_s(uint16_t duration_s)
{
    if (duration_s == 0U) {
        return 0U;
    }
    return (duration_s > WATER_MAX_DURATION_S) ? WATER_MAX_DURATION_S : (uint32_t)duration_s;
}

static void watering_gpio_init_once(void)
{
    if (s_gpio_initialized) {
        return;
    }

    (void)gpio_reset_pin(WATER_GPIO_NUM);
    (void)gpio_set_direction(WATER_GPIO_NUM, GPIO_MODE_OUTPUT);
    (void)gpio_set_level(WATER_GPIO_NUM, 0);
    s_gpio_initialized = true;
}

static void set_active(bool active)
{
    portENTER_CRITICAL(&s_lock);
    s_active = active;
    portEXIT_CRITICAL(&s_lock);
}

static bool get_active(void)
{
    bool active;
    portENTER_CRITICAL(&s_lock);
    active = s_active;
    portEXIT_CRITICAL(&s_lock);
    return active;
}

static void update_last_watering(time_t now)
{
    if (now > 0) {
        s_last_watering_s = (uint32_t)now;
    }
}

static void watering_stop_internal(void)
{
    (void)gpio_set_level(WATER_GPIO_NUM, 0);
    set_active(false);
}

static void watering_timer_cb(void *arg)
{
    (void)arg;
    watering_stop_internal();
    if (s_loop != NULL) {
        (void)esp_event_post_to(s_loop, APP_EVENTS, APP_EVENT_WATERING_DONE, NULL, 0, 0);
    }
}

static esp_err_t ensure_timer(void)
{
    if (s_timer != NULL) {
        return ESP_OK;
    }

    const esp_timer_create_args_t args = {
        .callback = watering_timer_cb,
        .name = "watering",
    };
    return esp_timer_create(&args, &s_timer);
}

/* =========================================================================
   SECTION: Public API
   ========================================================================= */
esp_err_t watering_manager_init(esp_event_loop_handle_t loop)
{
    s_loop = loop;
    return ESP_OK;
}

esp_err_t watering_manager_start_async(uint16_t duration_s)
{
    uint32_t clamped = clamp_duration_s(duration_s);
    if (clamped == 0U) {
        return ESP_ERR_INVALID_ARG;
    }

    if (get_active()) {
        ESP_LOGW(TAG, "watering already active");
        return ESP_ERR_INVALID_STATE;
    }

    watering_gpio_init_once();
    update_last_watering(time(NULL));
    set_active(true);
    (void)gpio_set_level(WATER_GPIO_NUM, 1);

    esp_err_t err = ensure_timer();
    if (err != ESP_OK) {
        watering_stop_internal();
        return err;
    }

    (void)esp_timer_stop(s_timer);
    err = esp_timer_start_once(s_timer, (uint64_t)clamped * 1000000ULL);
    if (err != ESP_OK) {
        watering_stop_internal();
        return err;
    }

    ESP_LOGI(TAG, "watering start async dur=%u", (unsigned)clamped);
    return ESP_OK;
}

esp_err_t watering_manager_start_blocking(uint16_t duration_s)
{
    uint32_t clamped = clamp_duration_s(duration_s);
    if (clamped == 0U) {
        return ESP_ERR_INVALID_ARG;
    }

    if (get_active()) {
        ESP_LOGW(TAG, "watering already active");
        return ESP_ERR_INVALID_STATE;
    }

    watering_gpio_init_once();
    update_last_watering(time(NULL));
    set_active(true);
    (void)gpio_set_level(WATER_GPIO_NUM, 1);
    vTaskDelay(pdMS_TO_TICKS(clamped * 1000U));
    watering_stop_internal();

    ESP_LOGI(TAG, "watering done blocking dur=%u", (unsigned)clamped);
    return ESP_OK;
}

void watering_manager_deinit(void)
{
    if (s_timer) {
        (void)esp_timer_stop(s_timer);
        (void)esp_timer_delete(s_timer);
        s_timer = NULL;
    }

    if (s_gpio_initialized) {
        (void)gpio_set_level(WATER_GPIO_NUM, 0);
    }

    set_active(false);
    s_loop = NULL;
}

bool watering_manager_is_active(void)
{
    return get_active();
}

uint32_t watering_manager_get_last_watering_s(void)
{
    return s_last_watering_s;
}

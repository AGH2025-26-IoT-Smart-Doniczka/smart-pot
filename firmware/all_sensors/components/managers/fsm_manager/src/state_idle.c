#include <stdio.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "bsp_init.h"
#include "ssd1306.h"
#include "app_context.h"
#include "fsm_manager.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "buttons_manager.h"
#include "fsm_state_callbacks.h"
#include "watering_manager.h"

/* =========================================================================
   SECTION: Constants
   ========================================================================= */
#define IDLE_TIMEOUT_FORCE_WAKE_MS 10000
#define IDLE_TIMEOUT_NORMAL_MS     1000

static const char *TAG = "STATE_IDLE";
static esp_timer_handle_t s_idle_timer;
static uint32_t s_idle_timeout_ms = IDLE_TIMEOUT_NORMAL_MS;

/* =========================================================================
    SECTION: Forward Declarations
    ========================================================================= */
static void idle_timer_start(void);
static void idle_timer_stop(void);
static void idle_shutdown_display(void);

/* =========================================================================
   SECTION: Helpers
   ========================================================================= */
static void idle_timeout_cb(void *arg)
{
    (void)arg;
    if (buttons_manager_is_any_pressed() || watering_manager_is_active()) {
        idle_timer_start();
        return;
    }
    app_context_set_display_on_wakeup(false);
    idle_shutdown_display();
    (void)fsm_manager_post_event(APP_EVENT_IDLE_TIMEOUT, NULL, 0, 0);
}

static void idle_timer_start(void)
{
    if (s_idle_timer == NULL) {
        const esp_timer_create_args_t args = {
            .callback = idle_timeout_cb,
            .name = "idle_timeout",
        };
        if (esp_timer_create(&args, &s_idle_timer) != ESP_OK) {
            return;
        }
    }

    (void)esp_timer_stop(s_idle_timer);

    uint32_t timeout_ms = s_idle_timeout_ms;
    if (timeout_ms == 0U) {
        timeout_ms = IDLE_TIMEOUT_NORMAL_MS;
    }

    (void)esp_timer_start_once(s_idle_timer, (uint64_t)timeout_ms * 1000ULL);
}

static void idle_timer_stop(void)
{
    if (s_idle_timer) {
        (void)esp_timer_stop(s_idle_timer);
    }
}

static void idle_shutdown_display(void)
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

/* =========================================================================
   SECTION: Callbacks
   ========================================================================= */
void state_idle_on_enter(void)
{
    ESP_LOGI(TAG, "enter");

    (void)mqtt_manager_stop();
    wifi_manager_stop();

    if (app_context_display_on_wakeup()) {
        s_idle_timeout_ms = IDLE_TIMEOUT_FORCE_WAKE_MS;
    } else {
        s_idle_timeout_ms = IDLE_TIMEOUT_NORMAL_MS;
    }

    idle_timer_start();
}

void state_idle_on_exit(exit_mode_t mode)
{
    (void)mode;
    idle_timer_stop();
    ESP_LOGI(TAG, "exit");
}

void state_idle_kick(void)
{
    idle_timer_start();
}
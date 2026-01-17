#include "esp_log.h"
#include "esp_timer.h"
#include "bsp_init.h"
#include "ssd1306.h"
#include "app_context.h"
#include "fsm_manager.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "fsm_state_callbacks.h"

/* =========================================================================
   SECTION: Constants
   ========================================================================= */
#define IDLE_TIMEOUT_MS 3000

static const char *TAG = "STATE_IDLE";
static esp_timer_handle_t s_idle_timer;

/* =========================================================================
   SECTION: Helpers
   ========================================================================= */
static void idle_timeout_cb(void *arg)
{
    (void)arg;
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
    (void)esp_timer_start_once(s_idle_timer, (uint64_t)IDLE_TIMEOUT_MS * 1000ULL);
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
    idle_shutdown_display();

    idle_timer_start();
}

void state_idle_on_exit(exit_mode_t mode)
{
    (void)mode;
    idle_timer_stop();
    ESP_LOGI(TAG, "exit");
}
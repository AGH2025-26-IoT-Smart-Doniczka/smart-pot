#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_event.h"
#include "bsp_init.h"
#include "ssd1306.h"
#include "buttons_manager.h"
#include "app_events.h"

ESP_EVENT_DEFINE_BASE(APP_EVENTS);

static const char *TAG = "BTN_DEMO";
static ssd1306_handle_t s_display;

/* =========================================================================
   SECTION: Helpers
   ========================================================================= */
static const char *event_name(app_event_id_t id)
{
    switch (id) {
        case APP_EVENT_BTN1_SHORT: return "BTN1 SHORT";
        case APP_EVENT_BTN1_3S:    return "BTN1 3S";
        case APP_EVENT_BTN1_10S:   return "BTN1 10S";
        case APP_EVENT_BTN2_SHORT: return "BTN2 SHORT";
        case APP_EVENT_BTN2_3S:    return "BTN2 3S";
        case APP_EVENT_BTN2_10S:   return "BTN2 10S";
        default:                   return "OTHER";
    }
}

static void draw_event(const char *text)
{
    if (s_display == NULL) {
        return;
    }
    (void)ssd1306_clear(s_display);
    (void)ssd1306_draw_text(s_display, "Button Demo", 0, 0);
    (void)ssd1306_draw_text(s_display, text, 0, 2);
    (void)ssd1306_flush(s_display);
}

static void app_evt_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    (void)handler_args;
    (void)event_data;
    if (base != APP_EVENTS) {
        return;
    }
    const char *name = event_name((app_event_id_t)event_id);
    ESP_LOGI(TAG, "Event: %s", name);
    draw_event(name);
}

/* =========================================================================
   SECTION: Public API
   ========================================================================= */
void app_main(void)
{
    i2c_master_bus_handle_t disp_bus = NULL;

    ESP_ERROR_CHECK(bsp_i2c_create_display_bus(&disp_bus));
    ESP_ERROR_CHECK(ssd1306_create(disp_bus, &s_display));
    draw_event("Waiting...");

    esp_event_loop_args_t loop_args = {
        .queue_size = 8,
        .task_name = "btn_evt",
        .task_priority = 4,
        .task_stack_size = 4096,
        .task_core_id = tskNO_AFFINITY,
    };

    esp_event_loop_handle_t loop = NULL;
    ESP_ERROR_CHECK(esp_event_loop_create(&loop_args, &loop));
    ESP_ERROR_CHECK(esp_event_handler_register_with(loop, APP_EVENTS, ESP_EVENT_ANY_ID, app_evt_handler, NULL));
    ESP_ERROR_CHECK(buttons_manager_init(loop));

    ESP_LOGI(TAG, "Button demo ready");

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

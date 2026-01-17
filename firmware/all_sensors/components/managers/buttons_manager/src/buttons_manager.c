#include <stdbool.h>
#include <stdint.h>
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_check.h"
#include "iot_button.h"
#include "board_pins.h"
#include "buttons_manager.h"
#include <button_gpio.h>

/* =========================================================================
   SECTION: Constants
   ========================================================================= */
#define BTN_DEBOUNCE_MS       30
#define BTN_SHORT_PRESS_TIME  50
#define BTN_HOLD_3S_MS        3000
#define BTN_HOLD_10S_MS       6000

/* =========================================================================
   SECTION: Structures
   ========================================================================= */
typedef struct {
    app_event_id_t ev_short;
    app_event_id_t ev_3s;
    app_event_id_t ev_10s; 
} btn_ctx_t;

/* =========================================================================
   SECTION: Static State
   ========================================================================= */
static const char *TAG = "BTN_MGR";
static esp_event_loop_handle_t s_loop;
static btn_ctx_t s_btn1;
static btn_ctx_t s_btn2;
static bool s_initialized;

/* =========================================================================
   SECTION: Helpers
   ========================================================================= */
static void post_event(app_event_id_t id) {
    if (s_loop == NULL) {
        return;
    }
    (void)esp_event_post_to(s_loop, APP_EVENTS, id, NULL, 0, 0);
}


static void button_single_click_cb(void *arg, void *data) {
    (void)arg;
    btn_ctx_t *ctx = (btn_ctx_t *)data;
    if (ctx == NULL) {
        return;
    }
    post_event(ctx->ev_short);
}

static void button_long_press_up_cb(void *arg, void *data) {
    btn_ctx_t *ctx = (btn_ctx_t *)data;
    if (ctx == NULL) {
        return;
    }

    uint32_t press_ms = iot_button_get_pressed_time((button_handle_t)arg);
    ESP_LOGD(TAG, "Button long press released after %lu ms", (unsigned long)press_ms);

    if (press_ms >= BTN_HOLD_10S_MS) {
        post_event(ctx->ev_10s);
    } else if (press_ms >= BTN_HOLD_3S_MS) {
        post_event(ctx->ev_3s);
    }
}

static esp_err_t init_button_gpio(btn_ctx_t *b,
                                  gpio_num_t pin,
                                  int active_level) {
    ESP_RETURN_ON_FALSE(b != NULL, ESP_ERR_INVALID_ARG, TAG, "ctx null");

    const button_config_t btn_cfg = {
        .long_press_time = BTN_HOLD_3S_MS,
        .short_press_time = BTN_SHORT_PRESS_TIME,
    };

    const button_gpio_config_t gpio_cfg = {
        .gpio_num = pin,
        .active_level = (uint8_t)active_level,
        .enable_power_save = true,
        .disable_pull = false,
    };

    button_handle_t gpio_btn = NULL;
    ESP_RETURN_ON_ERROR(iot_button_new_gpio_device(&btn_cfg, &gpio_cfg, &gpio_btn), TAG, "new gpio btn");

    ESP_RETURN_ON_ERROR(iot_button_register_cb(gpio_btn, BUTTON_SINGLE_CLICK, NULL, button_single_click_cb, b), TAG, "reg single");

    button_event_args_t long_args = {
        .long_press.press_time = BTN_HOLD_3S_MS,
    };
    ESP_RETURN_ON_ERROR(iot_button_register_cb(gpio_btn, BUTTON_LONG_PRESS_UP, &long_args, button_long_press_up_cb, b), TAG, "reg long up");

    return ESP_OK;
}

/* =========================================================================
   SECTION: Public API
   ========================================================================= */
esp_err_t buttons_manager_init(esp_event_loop_handle_t loop)
{
    ESP_RETURN_ON_FALSE(loop != NULL, ESP_ERR_INVALID_ARG, TAG, "loop null");
    if (s_initialized) {
        return ESP_OK;
    }

    s_loop = loop;

    s_btn1 = (btn_ctx_t){
        .ev_short = APP_EVENT_BTN1_SHORT,
        .ev_3s = APP_EVENT_BTN1_3S,
        .ev_10s = APP_EVENT_BTN1_10S,
    };
    s_btn2 = (btn_ctx_t){
        .ev_short = APP_EVENT_BTN2_SHORT,
        .ev_3s = APP_EVENT_BTN2_3S,
        .ev_10s = APP_EVENT_BTN2_10S,
    };

    ESP_RETURN_ON_ERROR(init_button_gpio(&s_btn1, BSP_BTN1_PIN, BSP_BTN1_ACTIVE_LEVEL), TAG, "btn1 init");
    ESP_RETURN_ON_ERROR(init_button_gpio(&s_btn2, BSP_BTN2_PIN, BSP_BTN2_ACTIVE_LEVEL), TAG, "btn2 init");

    s_initialized = true;
    ESP_LOGI(TAG, "Buttons manager started (BTN1=%d, BTN2=%d, active_level=%d)", (int)BSP_BTN1_PIN, (int)BSP_BTN2_PIN, BSP_BTN1_ACTIVE_LEVEL);
    return ESP_OK;
}

esp_err_t buttons_manager_enable_deep_sleep_wakeup(void)
{
    return esp_sleep_enable_ext0_wakeup(BSP_BTN1_PIN, BSP_BTN1_ACTIVE_LEVEL);
}
    

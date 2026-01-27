#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_check.h"
#include "fsm_manager.h"
#include "fsm_state_callbacks.h"
#include "app_context.h"
#include "nvs_manager.h"
#include "bsp_init.h"
#include "ssd1306.h"
#include "ssd1306_images.h"
#include "buttons_manager.h"
#include "mqtt_manager.h"

ESP_EVENT_DEFINE_BASE(APP_EVENTS);

static const char *TAG = "FSM";


/* =========================================================================
   SECTION: Internal Types
   ========================================================================= */
typedef struct {
    app_state_t state;
    esp_event_loop_handle_t loop;
    fsm_callbacks_t callbacks;
    bool initialized;
} fsm_context_t;

/* =========================================================================
   SECTION: Static Data
   ========================================================================= */
static fsm_context_t s_fsm;

/* =========================================================================
   SECTION: Helpers
   ========================================================================= */
static void fsm_shutdown_display(void)
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

static const char *fsm_event_str(app_event_id_t event_id)
{
    switch (event_id) {
        case APP_EVENT_CONFIG_LOADED: return "CONFIG_LOADED";
        case APP_EVENT_NO_CONFIG: return "NO_CONFIG";
        case APP_EVENT_PROV_CONNECTED: return "PROV_CONNECTED";
        case APP_EVENT_PROV_DATA_RECVD: return "PROV_DATA_RECVD";
        case APP_EVENT_PROV_TIMEOUT: return "PROV_TIMEOUT";
        case APP_EVENT_REQUIRES_CALIBRATION: return "REQUIRES_CALIBRATION";
        case APP_EVENT_WIFI_CONNECTED: return "WIFI_CONNECTED";
        case APP_EVENT_WIFI_DISCONNECTED: return "WIFI_DISCONNECTED";
        case APP_EVENT_TIME_SYNC_DONE: return "TIME_SYNC_DONE";
        case APP_EVENT_SENSORS_DATA_READY: return "SENSORS_DATA_READY";
        case APP_EVENT_CALIB_TIMEOUT: return "CALIB_TIMEOUT";
        case APP_EVENT_DECISION_MQTT: return "DECISION_MQTT";
        case APP_EVENT_DECISION_STORAGE: return "DECISION_STORAGE";
        case APP_EVENT_MQTT_PUBLISHED: return "MQTT_PUBLISHED";
        case APP_EVENT_STORAGE_SAVED: return "STORAGE_SAVED";
        case APP_EVENT_IDLE_TIMEOUT: return "IDLE_TIMEOUT";
        case APP_EVENT_BTN1_SHORT: return "BTN1_SHORT";
        case APP_EVENT_BTN1_3S: return "BTN1_3S";
        case APP_EVENT_BTN1_10S: return "BTN1_10S";
        case APP_EVENT_BTN2_SHORT: return "BTN2_SHORT";
        case APP_EVENT_BTN2_3S: return "BTN2_3S";
        case APP_EVENT_BTN2_10S: return "BTN2_10S";
        case APP_EVENT_FACTORY_RESET_DONE: return "FACTORY_RESET_DONE";
        default: return "UNKNOWN";
    }
}

static void fsm_invoke_entry_action(app_state_t state)
{
    switch (state) {
        case STATE_INIT:
            if (s_fsm.callbacks.init.on_enter) {
                s_fsm.callbacks.init.on_enter();
            }
            break;
        case STATE_CALIB_SOIL_DRY:
            if (s_fsm.callbacks.calib_soil_dry.on_enter) {
                s_fsm.callbacks.calib_soil_dry.on_enter();
            }
            break;
        case STATE_CALIB_SOIL_WET:
            if (s_fsm.callbacks.calib_soil_wet.on_enter) {
                s_fsm.callbacks.calib_soil_wet.on_enter();
            }
            break;
        case STATE_PROVISIONING:
            if (s_fsm.callbacks.provisioning.on_enter) {
                s_fsm.callbacks.provisioning.on_enter();
            }
            break;
        case STATE_WIFI_CONNECT:
            if (s_fsm.callbacks.wifi_connect.on_enter) {
                s_fsm.callbacks.wifi_connect.on_enter();
            }
            break;
        case STATE_SYNC_TIME:
            if (s_fsm.callbacks.sync_time.on_enter) {
                s_fsm.callbacks.sync_time.on_enter();
            }
            break;
        case STATE_SENSING:
            if (s_fsm.callbacks.sensing.on_enter) {
                s_fsm.callbacks.sensing.on_enter();
            }
            break;
        case STATE_DATA_DECISION:
            if (s_fsm.callbacks.data_decision.on_enter) {
                s_fsm.callbacks.data_decision.on_enter();
            }
            break;
        case STATE_MQTT_PUBLISH:
            if (s_fsm.callbacks.mqtt_publish.on_enter) {
                s_fsm.callbacks.mqtt_publish.on_enter();
            }
            break;
        case STATE_FLASH_STORE:
            if (s_fsm.callbacks.flash_store.on_enter) {
                s_fsm.callbacks.flash_store.on_enter();
            }
            break;
        case STATE_FACTORY_RESET:
            if (s_fsm.callbacks.factory_reset.on_enter) {
                s_fsm.callbacks.factory_reset.on_enter();
            }
            break;
        case STATE_IDLE:
            if (s_fsm.callbacks.idle.on_enter) {
                s_fsm.callbacks.idle.on_enter();
            }
            break;
        case STATE_DEEP_SLEEP:
            if (s_fsm.callbacks.deep_sleep.on_enter) {
                s_fsm.callbacks.deep_sleep.on_enter();
            }
            break;
        default:
            break;
    }
}

static void fsm_invoke_exit_action(app_state_t state, exit_mode_t mode)
{
    switch (state) {
        case STATE_INIT:
            if (s_fsm.callbacks.init.on_exit) {
                s_fsm.callbacks.init.on_exit(mode);
            }
            break;
        case STATE_CALIB_SOIL_DRY:
            if (s_fsm.callbacks.calib_soil_dry.on_exit) {
                s_fsm.callbacks.calib_soil_dry.on_exit(mode);
            }
            break;
        case STATE_CALIB_SOIL_WET:
            if (s_fsm.callbacks.calib_soil_wet.on_exit) {
                s_fsm.callbacks.calib_soil_wet.on_exit(mode);
            }
            break;
        case STATE_PROVISIONING:
            if (s_fsm.callbacks.provisioning.on_exit) {
                s_fsm.callbacks.provisioning.on_exit(mode);
            }
            break;
        case STATE_WIFI_CONNECT:
            if (s_fsm.callbacks.wifi_connect.on_exit) {
                s_fsm.callbacks.wifi_connect.on_exit(mode);
            }
            break;
        case STATE_SYNC_TIME:
            if (s_fsm.callbacks.sync_time.on_exit) {
                s_fsm.callbacks.sync_time.on_exit(mode);
            }
            break;
        case STATE_SENSING:
            if (s_fsm.callbacks.sensing.on_exit) {
                s_fsm.callbacks.sensing.on_exit(mode);
            }
            break;
        case STATE_DATA_DECISION:
            if (s_fsm.callbacks.data_decision.on_exit) {
                s_fsm.callbacks.data_decision.on_exit(mode);
            }
            break;
        case STATE_MQTT_PUBLISH:
            if (s_fsm.callbacks.mqtt_publish.on_exit) {
                s_fsm.callbacks.mqtt_publish.on_exit(mode);
            }
            break;
        case STATE_FLASH_STORE:
            if (s_fsm.callbacks.flash_store.on_exit) {
                s_fsm.callbacks.flash_store.on_exit(mode);
            }
            break;
        case STATE_FACTORY_RESET:
            if (s_fsm.callbacks.factory_reset.on_exit) {
                s_fsm.callbacks.factory_reset.on_exit(mode);
            }
            break;
        case STATE_IDLE:
            if (s_fsm.callbacks.idle.on_exit) {
                s_fsm.callbacks.idle.on_exit(mode);
            }
            break;
        case STATE_DEEP_SLEEP:
            if (s_fsm.callbacks.deep_sleep.on_exit) {
                s_fsm.callbacks.deep_sleep.on_exit(mode);
            }
            break;
        default:
            break;
    }
}

static void fsm_transition_internal(app_state_t next_state, const char *reason, bool force)
{
    if (!force && s_fsm.state == next_state) {
        ESP_LOGD(TAG, "State unchanged: %s", app_state_str(next_state));
        return;
    }

    exit_mode_t mode = force ? EXIT_MODE_INTERRUPTED : EXIT_MODE_DEFAULT;
    fsm_invoke_exit_action(s_fsm.state, mode);
    ESP_LOGI(TAG, "State %s -> %s (%s)", app_state_str(s_fsm.state), app_state_str(next_state), reason);
    s_fsm.state = next_state;
    fsm_invoke_entry_action(next_state);
}

static void fsm_transition(app_state_t next_state, const char *reason)
{
    fsm_transition_internal(next_state, reason, false);
}

static void fsm_transition_force(app_state_t next_state, const char *reason)
{
    fsm_transition_internal(next_state, reason, true);
}

static bool fsm_handle_global_interrupts(app_event_id_t event_id)
{
    if (event_id == APP_EVENT_BTN1_10S) {
        fsm_shutdown_display();
        (void)mqtt_manager_stop();
        fsm_transition_force(STATE_FACTORY_RESET, "button 10s");
        return true;
    }

    if (event_id == APP_EVENT_BTN1_3S) {
        fsm_shutdown_display();
        (void)mqtt_manager_stop();
        fsm_transition(STATE_PROVISIONING, "button 3s");
        return true;
    }

    return false;
}

static void fsm_handle_state_init(app_event_id_t event_id)
{
    switch (event_id) {
        case APP_EVENT_CONFIG_LOADED:
            fsm_transition(STATE_SENSING, "config loaded");
            break;
        case APP_EVENT_NO_CONFIG:
            fsm_transition(STATE_CALIB_SOIL_DRY, "config missing -> calibrate");
            break;
        default:
            ESP_LOGW(TAG, "INIT ignoring event %s", fsm_event_str(event_id));
            break;
    }
}

static void fsm_handle_state_calib(app_state_t state, app_event_id_t event_id)
{
    switch (event_id) {
        case APP_EVENT_CALIB_TIMEOUT:
            fsm_transition(STATE_DEEP_SLEEP, "calibration timeout");
            break;
        case APP_EVENT_BTN2_SHORT:
            if (state == STATE_CALIB_SOIL_DRY) {
                fsm_transition(STATE_CALIB_SOIL_WET, "cal dry -> wet");
            } else if (state == STATE_CALIB_SOIL_WET) {
                fsm_transition(STATE_PROVISIONING, "cal wet -> provisioning");
            }
            break;
        case APP_EVENT_BTN1_SHORT:
            if (state == STATE_CALIB_SOIL_WET) {
                fsm_transition(STATE_CALIB_SOIL_DRY, "cal wet -> dry");
            }
            break;
        default:
            ESP_LOGW(TAG, "CALIB state %s ignoring event %s", app_state_str(state), fsm_event_str(event_id));
            break;
    }
}

static void fsm_handle_state_provisioning(app_event_id_t event_id)
{
    switch (event_id) {
        case APP_EVENT_REQUIRES_CALIBRATION:
            fsm_transition(STATE_CALIB_SOIL_DRY, "calibration required");
            break;
        case APP_EVENT_PROV_DATA_RECVD:
            fsm_transition(STATE_SENSING, "provisioned");
            break;
        case APP_EVENT_PROV_TIMEOUT:
            fsm_transition(STATE_DEEP_SLEEP, "provisioning timeout");
            break;
        default:
            ESP_LOGW(TAG, "PROVISIONING ignoring event %s", fsm_event_str(event_id));
            break;
    }
}

static void fsm_handle_state_wifi_connect(app_event_id_t event_id)
{
    switch (event_id) {
        case APP_EVENT_WIFI_CONNECTED:
            fsm_transition(STATE_SYNC_TIME, "wifi connected");
            break;
        case APP_EVENT_WIFI_DISCONNECTED:
            fsm_transition(STATE_DATA_DECISION, "wifi unavailable");
            break;
        default:
            ESP_LOGW(TAG, "WIFI_CONNECT ignoring event %s", fsm_event_str(event_id));
            break;
    }
}

static void fsm_handle_state_sync_time(app_event_id_t event_id)
{
    switch (event_id) {
        case APP_EVENT_TIME_SYNC_DONE:
            fsm_transition(STATE_DATA_DECISION, "time sync done");
            break;
        case APP_EVENT_WIFI_DISCONNECTED:
            fsm_transition(STATE_DATA_DECISION, "sync offline");
            break;
        default:
            ESP_LOGW(TAG, "SYNC_TIME ignoring event %s", fsm_event_str(event_id));
            break;
    }
}

static void fsm_handle_state_sensing(app_event_id_t event_id)
{
    switch (event_id) {
        case APP_EVENT_SENSORS_DATA_READY:
            fsm_transition(STATE_WIFI_CONNECT, "sensing done");
            break;
        default:
            ESP_LOGW(TAG, "SENSING ignoring event %s", fsm_event_str(event_id));
            break;
    }
}

static void fsm_handle_state_decision(app_event_id_t event_id)
{
    switch (event_id) {
        case APP_EVENT_DECISION_MQTT:
            fsm_transition(STATE_MQTT_PUBLISH, "decision mqtt");
            break;
        case APP_EVENT_DECISION_STORAGE:
            fsm_transition(STATE_FLASH_STORE, "decision storage");
            break;
        case APP_EVENT_WIFI_DISCONNECTED:
            fsm_transition(STATE_FLASH_STORE, "offline, store");
            break;
        default:
            ESP_LOGW(TAG, "DATA_DECISION ignoring event %s", fsm_event_str(event_id));
            break;
    }
}

static void fsm_handle_state_mqtt_publish(app_event_id_t event_id)
{
    switch (event_id) {
        case APP_EVENT_MQTT_PUBLISHED:
            fsm_transition(STATE_IDLE, "mqtt published");
            break;
        case APP_EVENT_DECISION_STORAGE:
            fsm_transition(STATE_FLASH_STORE, "mqtt failures -> store");
            break;
        case APP_EVENT_WIFI_DISCONNECTED:
            fsm_transition(STATE_FLASH_STORE, "mqtt fallback store");
            break;
        default:
            ESP_LOGW(TAG, "MQTT_PUBLISH ignoring event %s", fsm_event_str(event_id));
            break;
    }
}

static void fsm_handle_state_flash_store(app_event_id_t event_id)
{
    switch (event_id) {
        case APP_EVENT_STORAGE_SAVED:
            fsm_transition(STATE_IDLE, "storage saved");
            break;
        default:
            ESP_LOGW(TAG, "FLASH_STORE ignoring event %s", fsm_event_str(event_id));
            break;
    }
}

static void fsm_handle_state_idle(app_event_id_t event_id)
{
    switch (event_id) {
        case APP_EVENT_IDLE_TIMEOUT:
            fsm_transition(STATE_DEEP_SLEEP, "idle timeout");
            break;
        default:
            ESP_LOGW(TAG, "IDLE ignoring event %s", fsm_event_str(event_id));
            break;
    }
}

static void fsm_handle_state_factory_reset(app_event_id_t event_id)
{
    switch (event_id) {
        case APP_EVENT_FACTORY_RESET_DONE:
            fsm_transition(STATE_INIT, "factory reset done");
            break;
        default:
            ESP_LOGW(TAG, "FACTORY_RESET ignoring event %s", fsm_event_str(event_id));
            break;
    }
}

static void fsm_dispatch_event(app_event_id_t event_id)
{
    switch (s_fsm.state) {
        case STATE_INIT:
            fsm_handle_state_init(event_id);
            break;
        case STATE_CALIB_SOIL_DRY:
        case STATE_CALIB_SOIL_WET:
            fsm_handle_state_calib(s_fsm.state, event_id);
            break;
        case STATE_PROVISIONING:
            fsm_handle_state_provisioning(event_id);
            break;
        case STATE_WIFI_CONNECT:
            fsm_handle_state_wifi_connect(event_id);
            break;
        case STATE_SYNC_TIME:
            fsm_handle_state_sync_time(event_id);
            break;
        case STATE_SENSING:
            fsm_handle_state_sensing(event_id);
            break;
        case STATE_DATA_DECISION:
            fsm_handle_state_decision(event_id);
            break;
        case STATE_MQTT_PUBLISH:
            fsm_handle_state_mqtt_publish(event_id);
            break;
        case STATE_FLASH_STORE:
            fsm_handle_state_flash_store(event_id);
            break;
        case STATE_FACTORY_RESET:
            fsm_handle_state_factory_reset(event_id);
            break;
        case STATE_IDLE:
            fsm_handle_state_idle(event_id);
            break;
        case STATE_DEEP_SLEEP:
            ESP_LOGW(TAG, "DEEP_SLEEP ignoring event %s", fsm_event_str(event_id));
            break;
        default:
            ESP_LOGE(TAG, "Unknown state %d", (int)s_fsm.state);
            break;
    }
}

static void fsm_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    (void) handler_args;
    (void) event_data;

    if (base != APP_EVENTS) {
        return;
    }

    app_event_id_t id = (app_event_id_t)event_id;
    if (fsm_handle_global_interrupts(id)) {
        return;
    }

    fsm_dispatch_event(id);
}

/* =========================================================================
   SECTION: Public API
   ========================================================================= */
esp_err_t fsm_manager_init(const fsm_callbacks_t *callbacks)
{
    if (s_fsm.initialized) {
        return ESP_OK;
    }

    memset(&s_fsm, 0, sizeof(s_fsm));
    s_fsm.state = STATE_INIT;

    const fsm_callbacks_t *cb_src = (callbacks != NULL) ? callbacks : fsm_state_callbacks_get();
    s_fsm.callbacks = *cb_src;

    esp_event_loop_args_t loop_args = {
        .queue_size = 16,
        .task_name = "fsm_evt",
        .task_priority = 4,
        .task_stack_size = 4096,
        .task_core_id = tskNO_AFFINITY
    };

    esp_err_t err = esp_event_loop_create(&loop_args, &s_fsm.loop);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create event loop (%s)", esp_err_to_name(err));
        return err;
    }

    err = esp_event_handler_register_with(s_fsm.loop, APP_EVENTS, ESP_EVENT_ANY_ID, fsm_event_handler, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register handler (%s)", esp_err_to_name(err));
        esp_event_loop_delete(s_fsm.loop);
        s_fsm.loop = NULL;
        return err;
    }

    err = buttons_manager_init(s_fsm.loop);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Buttons manager init failed (%s)", esp_err_to_name(err));
        return err;
    }

    s_fsm.initialized = true;
    fsm_invoke_entry_action(STATE_INIT);
    ESP_LOGI(TAG, "FSM initialized, state %s", app_state_str(s_fsm.state));
    return ESP_OK;
}

esp_err_t fsm_manager_post_event(app_event_id_t event_id,
                                 const void *event_data,
                                 size_t event_data_size,
                                 uint32_t timeout_ms)
{
    if (!s_fsm.initialized || s_fsm.loop == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    TickType_t timeout_ticks = (timeout_ms == UINT32_MAX) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return esp_event_post_to(s_fsm.loop, APP_EVENTS, event_id, event_data, event_data_size, timeout_ticks);
}

app_state_t fsm_manager_get_state(void)
{
    return s_fsm.state;
}

esp_err_t fsm_manager_set_callbacks(const fsm_callbacks_t *callbacks)
{
    if (!s_fsm.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (callbacks == NULL) {
        s_fsm.callbacks = *fsm_state_callbacks_get();
        return ESP_OK;
    }

    s_fsm.callbacks = *callbacks;
    return ESP_OK;
}

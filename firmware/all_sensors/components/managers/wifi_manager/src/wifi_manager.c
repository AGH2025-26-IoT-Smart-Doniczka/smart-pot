#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_wifi.h"
#include "app_context.h"
#include "app_events.h"
#include "fsm_manager.h"
#include "wifi_manager.h"

/* =========================================================================
   SECTION: Constants
   ========================================================================= */
#define WIFI_MAX_RETRY 5

/* =========================================================================
   SECTION: Static Data
   ========================================================================= */
static const char *TAG = "WIFI_MGR";
static bool s_initialized;
static bool s_started;
static bool s_sntp_started;
static int s_retry_num;
static esp_netif_t *s_sta_netif;
static esp_event_handler_instance_t s_any_id_handler;
static esp_event_handler_instance_t s_got_ip_handler;

/* =========================================================================
   SECTION: Helpers
   ========================================================================= */
static void sntp_sync_cb(struct timeval *tv)
{
    (void)tv;
    app_context_set_time_synced(true);
    (void)fsm_manager_post_event(APP_EVENT_TIME_SYNC_DONE, NULL, 0, 0);
    ESP_LOGI(TAG, "time sync done");
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;
    (void)event_data;

    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                s_retry_num = 0;
                (void)esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                app_context_set_wifi_connected(false);
                if (s_started && s_retry_num < WIFI_MAX_RETRY) {
                    s_retry_num++;
                    (void)esp_wifi_connect();
                } else {
                    (void)fsm_manager_post_event(APP_EVENT_WIFI_DISCONNECTED, NULL, 0, 0);
                }
                break;
            default:
                break;
        }
        return;
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        s_retry_num = 0;
        app_context_set_wifi_connected(true);
        (void)fsm_manager_post_event(APP_EVENT_WIFI_CONNECTED, NULL, 0, 0);
    }
}

static esp_err_t ensure_event_loop(void)
{
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    if (s_sta_netif == NULL) {
        s_sta_netif = esp_netif_create_default_wifi_sta();
        if (s_sta_netif == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }

    return ESP_OK;
}

static esp_err_t register_handlers(void)
{
    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(WIFI_EVENT,
                                                           ESP_EVENT_ANY_ID,
                                                           &wifi_event_handler,
                                                           NULL,
                                                           &s_any_id_handler),
                        TAG, "wifi handler");
    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(IP_EVENT,
                                                           IP_EVENT_STA_GOT_IP,
                                                           &wifi_event_handler,
                                                           NULL,
                                                           &s_got_ip_handler),
                        TAG, "ip handler");
    return ESP_OK;
}

/* =========================================================================
   SECTION: Public API
   ========================================================================= */
esp_err_t wifi_manager_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(ensure_event_loop(), TAG, "netif");

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "wifi init");

    ESP_RETURN_ON_ERROR(register_handlers(), TAG, "handlers");

    s_initialized = true;
    ESP_LOGI(TAG, "initialized");
    return ESP_OK;
}

esp_err_t wifi_manager_start(void)
{
    ESP_RETURN_ON_ERROR(wifi_manager_init(), TAG, "init");

    config_t cfg = {0};
    if (app_context_get_config(&cfg) != ESP_OK) {
        ESP_LOGW(TAG, "config unavailable");
        return ESP_ERR_INVALID_STATE;
    }

    if (cfg.ssid[0] == '\0') {
        ESP_LOGW(TAG, "ssid empty");
        return ESP_ERR_INVALID_ARG;
    }

    wifi_config_t wifi_cfg = {0};
    (void)strncpy((char *)wifi_cfg.sta.ssid, cfg.ssid, sizeof(wifi_cfg.sta.ssid) - 1U);
    (void)strncpy((char *)wifi_cfg.sta.password, cfg.passwd, sizeof(wifi_cfg.sta.password) - 1U);

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "set mode");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg), TAG, "set config");

    if (s_started) {
        (void)esp_wifi_disconnect();
        (void)esp_wifi_connect();
        app_context_set_wifi_connected(false);
        s_retry_num = 0;
        ESP_LOGI(TAG, "wifi reconnect");
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "start");

    app_context_set_wifi_connected(false);
    s_started = true;
    ESP_LOGI(TAG, "wifi start");
    return ESP_OK;
}

void wifi_manager_stop(void)
{
    if (!s_started) {
        return;
    }

    (void)esp_wifi_stop();
    app_context_set_wifi_connected(false);
    s_started = false;
    ESP_LOGI(TAG, "wifi stop");
}

esp_err_t wifi_manager_request_time_sync(void)
{
    if (!app_context_is_wifi_connected()) {
        ESP_LOGW(TAG, "time sync requested without wifi");
        return ESP_ERR_INVALID_STATE;
    }

    if (app_context_is_time_synced()) {
        return ESP_OK;
    }

    if (s_sntp_started) {
        return ESP_OK;
    }

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(sntp_sync_cb);
    sntp_init();

    s_sntp_started = true;
    ESP_LOGI(TAG, "sntp start");
    return ESP_OK;
}

#include <string.h>
#include <time.h>
#include <stdint.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_mac.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include "app_context.h"
#include "app_types.h"
#include "app_constants.h"
#include "fsm_manager.h"
#include "mqtt_manager.h"
#include "nvs_manager.h"
#include "json_config_parser.h"
#include "watering_manager.h"

/* =========================================================================
   SECTION: Constants
   ========================================================================= */
#define MQTT_BROKER_HOST          "192.168.0.220"
#define MQTT_BROKER_PORT          1883

#define MQTT_TOPIC_BUF_LEN        96
#define MQTT_PAYLOAD_BUF_LEN      256
#define MQTT_FAIL_WINDOW_US       (30LL * 1000LL * 1000LL)
#define MQTT_FAIL_THRESHOLD       3
#define MQTT_TIME_VALID_EPOCH_S   1700000000UL

/* =========================================================================
   SECTION: Static Data
   ========================================================================= */
static const char *TAG = "MQTT_MGR";
static esp_mqtt_client_handle_t s_client = NULL;
static bool s_subscribed = false;
static bool s_publish_pending = false;
static bool s_connected = false;
static int s_last_pub_id = -1;
static char s_uuid[13] = {0};
static uint32_t s_device_id = 0;
static char s_mqtt_pass[33] = {0};
static int s_mqtt_fail_count = 0;
static int64_t s_mqtt_fail_window_start_us = 0;
static uint32_t s_mqtt_msg_counter = 0;

/* =========================================================================
   SECTION: Helpers
   ========================================================================= */
static void mqtt_build_uuid(void)
{
    if (s_uuid[0] != '\0') {
        return;
    }

    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_BT);
    (void)snprintf(s_uuid, sizeof(s_uuid),
                   "%02X%02X%02X%02X%02X%02X",
                   mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    s_device_id = ((uint32_t)mac[2] << 24) | ((uint32_t)mac[3] << 16) | ((uint32_t)mac[4] << 8) | (uint32_t)mac[5];
}

static void mqtt_build_topics(char *telemetry, size_t telemetry_len,
                              char *cfg_topic, size_t cfg_topic_len,
                              char *actions, size_t actions_len,
                              char *hard_reset, size_t hard_reset_len)
{
    mqtt_build_uuid();
    if (telemetry != NULL && telemetry_len > 0U) {
        (void)snprintf(telemetry, telemetry_len, "devices/%s/telemetry", s_uuid);
    }
    if (cfg_topic != NULL && cfg_topic_len > 0U) {
        (void)snprintf(cfg_topic, cfg_topic_len, "devices/%s/config", s_uuid);
    }
    if (actions != NULL && actions_len > 0U) {
        (void)snprintf(actions, actions_len, "devices/%s/actions", s_uuid);
    }
    if (hard_reset != NULL && hard_reset_len > 0U) {
        (void)snprintf(hard_reset, hard_reset_len, "devices/%s/hard-reset", s_uuid);
    }

}


static esp_err_t mqtt_publish_json(const char *topic, const char *payload, int qos)
{
    if ((s_client == NULL) || (topic == NULL) || (payload == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "publishing topic=%s qos=%d payload=%s", topic, qos, payload);
    int msg_id = esp_mqtt_client_publish(s_client, topic, payload, 0, qos, 0);
    if (msg_id < 0) {
        ESP_LOGW(TAG, "publish failed topic=%s", topic);
        return ESP_FAIL;
    }

    s_last_pub_id = msg_id;
    ESP_LOGI(TAG, "publish queued msg_id=%d", msg_id);
    return ESP_OK;
}


static uint32_t mqtt_next_timestamp_s(uint32_t unix_ts)
{
    return unix_ts + s_mqtt_msg_counter++;
}

static uint32_t mqtt_get_unix_ts(void)
{
    time_t now = time(NULL);
    if (now > (time_t)MQTT_TIME_VALID_EPOCH_S) {
        return (uint32_t)now;
    }
    if (app_context_is_time_synced()) {
        ESP_LOGW(TAG, "time synced flag set but time invalid (%ld)", (long)now);
    }
    return 0U;
}


/* =========================================================================
   SECTION: Topic Handlers
   ========================================================================= */
static void mqtt_handle_actions(const cJSON *root)
{
    const cJSON *typ = cJSON_GetObjectItem(root, "typ");
    const cJSON *data = cJSON_GetObjectItem(root, "data");
    if (cJSON_IsString(typ)) {
        ESP_LOGI(TAG, "action typ=%s", typ->valuestring);
    } else {
        ESP_LOGI(TAG, "action typ=<invalid>");
    }
    if (cJSON_IsObject(data)) {
        char *data_json = cJSON_PrintUnformatted(data);
        if (data_json != NULL) {
            ESP_LOGI(TAG, "action data=%s", data_json);
            cJSON_free(data_json);
        } else {
            ESP_LOGI(TAG, "action data=<unavailable>");
        }
    } else {
        ESP_LOGI(TAG, "action data=<invalid>");
    }
    if (cJSON_IsString(typ) && strcmp(typ->valuestring, "wtr") == 0) {
        if (!cJSON_IsObject(data)) {
            ESP_LOGW(TAG, "wtr action missing data");
            return;
        }

        const cJSON *dur = cJSON_GetObjectItem(data, "dur");
        if (!cJSON_IsNumber(dur)) {
            ESP_LOGW(TAG, "wtr action invalid duration");
            return;
        }

        int duration = (int)dur->valuedouble;
        if (duration < 1) {
            ESP_LOGW(TAG, "wtr action duration too small (%d)", duration);
            return;
        }
        if (duration > 60) {
            duration = 60;
        }

        esp_err_t err = watering_manager_start_async((uint16_t)duration);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "wtr action start failed (%s)", esp_err_to_name(err));
            return;
        }
        ESP_LOGI(TAG, "wtr action started dur=%d", duration);
    }
}

static void mqtt_handle_cfg(const char *json_str)
{
    config_t cfg = {0};
    if (app_context_get_config(&cfg) != ESP_OK) {
        ESP_LOGW(TAG, "config read failed");
        return;
    }

    esp_err_t err = json_config_parse(json_str, &cfg);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "cfg parse failed (%s)", esp_err_to_name(err));
        return;
    }

    if (app_context_set_config(&cfg) == ESP_OK) {
        ESP_LOGI(TAG, "config updated from mqtt");
        esp_err_t save_err = nvs_manager_save_config(&cfg);
        if (save_err == ESP_OK) {
            ESP_LOGI(TAG, "config saved to nvs");
        } else {
            ESP_LOGW(TAG, "config save failed (%s)", esp_err_to_name(save_err));
        }
    }
}

static void mqtt_handle_hard_reset(void)
{
    ESP_LOGE(TAG, "HARD_RESET");
    (void)fsm_manager_post_event(APP_EVENT_BTN1_10S, NULL, 0, 0);

}


static void mqtt_publish_telemetry_internal(void)
{
    sensor_data_t data = {0};
    (void)app_context_get_sensor_data(&data);

    char topic[MQTT_TOPIC_BUF_LEN] = {0};
    char actions_topic[MQTT_TOPIC_BUF_LEN] = {0};
    char hard_reset[MQTT_TOPIC_BUF_LEN] = {0};
    mqtt_build_topics(topic, sizeof(topic), NULL, 0U, actions_topic, sizeof(actions_topic), hard_reset, sizeof(hard_reset));

    float temp_c = ((float)data.temperature / 10.0f) - 273.15f;
    double temp_c_1dp = (double)((int)(temp_c * 10.0f + (temp_c >= 0.0f ? 0.5f : -0.5f))) / 10.0;

    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return;
    }

    uint32_t unix_ts = mqtt_get_unix_ts();
    cJSON_AddNumberToObject(root, "ts", mqtt_next_timestamp_s(unix_ts));

    cJSON *payload = cJSON_AddObjectToObject(root, "data");
    if (payload != NULL) {
        cJSON_AddNumberToObject(payload, "lux", (int)data.lux_level);
        cJSON_AddNumberToObject(payload, "tem", temp_c_1dp);
        cJSON_AddNumberToObject(payload, "moi", (int)data.soil_moisture);
        cJSON_AddNumberToObject(payload, "pre", (int)data.pressure);
    }

    char *json = cJSON_PrintUnformatted(root);
    if (json != NULL) {
        (void)mqtt_publish_json(topic, json, 1);
        cJSON_free(json);
    }
    cJSON_Delete(root);
}

static esp_err_t mqtt_publish_sample_payload(const char *topic, const sensor_data_t *data, uint32_t timestamp)
{
    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    float temp_c = ((float)data->temperature / 10.0f) - 273.15f;
    double temp_c_1dp = (double)((int)(temp_c * 10.0f + (temp_c >= 0.0f ? 0.5f : -0.5f))) / 10.0;

    if (timestamp == 0U) {
        timestamp = mqtt_get_unix_ts();
    }

    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddNumberToObject(root, "ts", mqtt_next_timestamp_s(timestamp));
    cJSON *payload = cJSON_AddObjectToObject(root, "data");
    if (payload != NULL) {
        cJSON_AddNumberToObject(payload, "lux", (int)data->lux_level);
        cJSON_AddNumberToObject(payload, "tem", temp_c_1dp);
        cJSON_AddNumberToObject(payload, "moi", (int)data->soil_moisture);
        cJSON_AddNumberToObject(payload, "pre", (int)data->pressure);
    }

    char *json = cJSON_PrintUnformatted(root);
    if (json == NULL) {
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = mqtt_publish_json(topic, json, 1);
    cJSON_free(json);
    cJSON_Delete(root);
    return err;
}

static void mqtt_publish_stored_samples(void)
{
    sensor_sample_t samples[NVS_SENSOR_SAMPLES_N] = {0};
    size_t count = 0;
    if (nvs_manager_get_all_samples(samples, NVS_SENSOR_SAMPLES_N, &count) != ESP_OK) {
        ESP_LOGW(TAG, "failed to read stored samples");
        return;
    }
    if (count == 0) {
        return;
    }

    char topic[MQTT_TOPIC_BUF_LEN] = {0};
    char actions_topic[MQTT_TOPIC_BUF_LEN] = {0};
    char hard_reset[MQTT_TOPIC_BUF_LEN] = {0};
    mqtt_build_topics(topic, sizeof(topic), NULL, 0U, actions_topic, sizeof(actions_topic), hard_reset, sizeof(hard_reset));

    bool all_ok = true;
    for (size_t i = 0; i < count; ++i) {
        if (mqtt_publish_sample_payload(topic, &samples[i].data, samples[i].timestamp) != ESP_OK) {
            all_ok = false;
            break;
        }
    }

    if (all_ok) {
        (void)nvs_manager_clear_samples();
        ESP_LOGI(TAG, "published %u stored samples", (unsigned)count);
    }
}

static void mqtt_handle_event_data(const esp_mqtt_event_handle_t event)
{
    if (event == NULL || event->topic == NULL || event->data == NULL) {
        return;
    }

    char cfg_topic[MQTT_TOPIC_BUF_LEN] = {0};
    char actions_topic[MQTT_TOPIC_BUF_LEN] = {0};
    char hard_reset_topic[MQTT_TOPIC_BUF_LEN] = {0};
    mqtt_build_topics(NULL, 0U, cfg_topic, sizeof(cfg_topic), actions_topic, sizeof(actions_topic), hard_reset_topic, sizeof(hard_reset_topic));

    if ((event->topic_len <= 0) || (event->data_len <= 0)) {
        return;
    }

    const bool is_action = (strlen(actions_topic) == (size_t)event->topic_len) &&
                           (strncmp(event->topic, actions_topic, event->topic_len) == 0);
    const bool is_cfg = (strlen(cfg_topic) == (size_t)event->topic_len) &&
                        (strncmp(event->topic, cfg_topic, event->topic_len) == 0);
    const bool is_hard_reset = (strlen(hard_reset_topic) == (size_t)event->topic_len) &&
                               (strncmp(event->topic, hard_reset_topic, event->topic_len) == 0);

    if (!is_action && !is_cfg && !is_hard_reset) {
        return;
    }

    if (is_hard_reset) {
        mqtt_handle_hard_reset();
        return;
    }

    if (event->data_len > MQTT_PAYLOAD_BUF_LEN) {
        ESP_LOGW(TAG, "action payload too large (%d)", event->data_len);
        return;
    }

    if (is_cfg) {
        char cfg_json[MQTT_PAYLOAD_BUF_LEN + 1U] = {0};
        memcpy(cfg_json, event->data, event->data_len);
        cfg_json[event->data_len] = '\0';
        mqtt_handle_cfg(cfg_json);
        return;
    }

    cJSON *root = cJSON_ParseWithLength(event->data, event->data_len);
    if (root == NULL) {
        ESP_LOGW(TAG, "json parse failed");
        return;
    }

    if (is_action) {
        mqtt_handle_actions(root);
    }

    cJSON_Delete(root);
}

static void mqtt_track_failure_and_fallback(void)
{
    int64_t now_us = esp_timer_get_time();
    if ((s_mqtt_fail_window_start_us == 0) || ((now_us - s_mqtt_fail_window_start_us) > MQTT_FAIL_WINDOW_US)) {
        s_mqtt_fail_window_start_us = now_us;
        s_mqtt_fail_count = 0;
    }

    s_mqtt_fail_count++;
    if (s_mqtt_fail_count >= MQTT_FAIL_THRESHOLD) {
        ESP_LOGW(TAG, "mqtt failures=%d within window, fallback to flash", s_mqtt_fail_count);
        s_mqtt_fail_count = 0;
        s_mqtt_fail_window_start_us = 0;
        (void)fsm_manager_post_event(APP_EVENT_DECISION_STORAGE, NULL, 0, 0);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    (void)handler_args;
    (void)base;

    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    if (event == NULL) {
        return;
    }

    switch (event_id) {
        case MQTT_EVENT_CONNECTED: {
            ESP_LOGI(TAG, "mqtt connected");
            s_connected = true;
            s_mqtt_fail_count = 0;
            s_mqtt_fail_window_start_us = 0;

            mqtt_publish_stored_samples();

            if (!s_subscribed) {
                char cfg_topic[MQTT_TOPIC_BUF_LEN] = {0};
                char actions_topic[MQTT_TOPIC_BUF_LEN] = {0};
                char hard_reset_topic[MQTT_TOPIC_BUF_LEN] = {0};
                mqtt_build_topics(NULL, 0U, cfg_topic, sizeof(cfg_topic), actions_topic, sizeof(actions_topic), hard_reset_topic, sizeof(hard_reset_topic));

                (void)esp_mqtt_client_subscribe(s_client, cfg_topic, 1);
                (void)esp_mqtt_client_subscribe(s_client, actions_topic, 1);
                (void)esp_mqtt_client_subscribe(s_client, hard_reset_topic, 1);
                s_subscribed = true;
            }

            if (s_publish_pending) {
                mqtt_publish_telemetry_internal();
                s_publish_pending = false;
            }
            break;
        }
        case MQTT_EVENT_DATA:
            mqtt_handle_event_data(event);
            break;
        case MQTT_EVENT_PUBLISHED:
            if (event->msg_id == s_last_pub_id) {
                ESP_LOGI(TAG, "publish confirmed msg_id=%d", event->msg_id);
                (void)fsm_manager_post_event(APP_EVENT_MQTT_PUBLISHED, NULL, 0, 0);
            }
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGW(TAG, "mqtt error");
            s_connected = false;
            mqtt_track_failure_and_fallback();
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "mqtt disconnected");
            s_connected = false;
            mqtt_track_failure_and_fallback();
            break;
        default:
            break;
    }
}

static void mqtt_prepare_password(void)
{
    config_t cfg = {0};
    if (app_context_get_config(&cfg) != ESP_OK) {
        memset(s_mqtt_pass, 0, sizeof(s_mqtt_pass));
        return;
    }

    memcpy(s_mqtt_pass, cfg.mqtt_passwd, sizeof(cfg.mqtt_passwd));
    s_mqtt_pass[sizeof(s_mqtt_pass) - 1] = '\0';
}

static esp_err_t mqtt_client_start_internal(void)
{
    if (s_client != NULL) {
        return ESP_OK;
    }

    mqtt_prepare_password();
    mqtt_build_uuid();

    bool has_prior_connect = app_context_has_prior_connect();
    ESP_LOGI(TAG, "mqtt session clean_start=%u", has_prior_connect ? 0U : 1U);

    char mqtt_uri[128] = {0};
    snprintf(mqtt_uri, sizeof(mqtt_uri), "mqtt://%s", MQTT_BROKER_HOST);

    esp_mqtt_client_config_t cfg = {
        .broker = {
            .address = {
                .uri = mqtt_uri,
                .port = MQTT_BROKER_PORT,
            },
        },
        .credentials = {
            .client_id = s_uuid,
            .username = s_uuid,
            .authentication = {
                .password = s_mqtt_pass,
                
            },
        },
        .session = {
            .keepalive = 60,
            .disable_clean_session = has_prior_connect,
            .protocol_ver = MQTT_PROTOCOL_V_5,
        },
    };

    s_client = esp_mqtt_client_init(&cfg);
    if (s_client == NULL) {
        return ESP_FAIL;
    }

    esp_mqtt5_connection_property_config_t connect_property = {
        .session_expiry_interval = has_prior_connect ? 0xFFFFFFFF : 0,
        .maximum_packet_size = 1024,
        .receive_maximum = 65535,
        .topic_alias_maximum = 10,
    };
    esp_mqtt5_client_set_connect_property(s_client, &connect_property);

    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    return esp_mqtt_client_start(s_client);
}

/* =========================================================================
   SECTION: Public API
   ========================================================================= */
esp_err_t mqtt_manager_start(void)
{
    return mqtt_client_start_internal();
}

esp_err_t mqtt_manager_publish_telemetry(void)
{
    esp_err_t err = mqtt_client_start_internal();
    if (err != ESP_OK) {
        return err;
    }

    if (s_connected) {
        mqtt_publish_telemetry_internal();
    } else {
        s_publish_pending = true;
    }
    return ESP_OK;
}

esp_err_t mqtt_manager_stop(void)
{
    if (s_client == NULL) {
        return ESP_OK;
    }

    (void)esp_mqtt_client_stop(s_client);
    (void)esp_mqtt_client_destroy(s_client);
    s_client = NULL;
    s_subscribed = false;
    s_publish_pending = false;
    s_connected = false;
    s_last_pub_id = -1;
    s_mqtt_fail_count = 0;
    s_mqtt_fail_window_start_us = 0;
    return ESP_OK;
}

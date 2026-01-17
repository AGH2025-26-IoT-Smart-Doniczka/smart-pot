#include <string.h>
#include <time.h>
#include <stdint.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_mac.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include "driver/gpio.h"
#include "app_context.h"
#include "app_types.h"
#include "app_constants.h"
#include "fsm_manager.h"
#include "mqtt_manager.h"
#include "nvs_manager.h"

/* =========================================================================
   SECTION: Constants
   ========================================================================= */
#define MQTT_BROKER_HOST          "172.20.10.2"
#define MQTT_BROKER_PORT          1883
#define MQTT_WATER_GPIO           GPIO_NUM_2

#define MQTT_TOPIC_BUF_LEN        96
#define MQTT_PAYLOAD_BUF_LEN      256
#define MQTT_FAIL_WINDOW_US       (30LL * 1000LL * 1000LL)
#define MQTT_FAIL_THRESHOLD       3

/* =========================================================================
   SECTION: Static Data
   ========================================================================= */
static const char *TAG = "MQTT_MGR";
static esp_mqtt_client_handle_t s_client = NULL;
static bool s_subscribed = false;
static bool s_publish_pending = false;
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
                              char *setup, size_t setup_len,
                              char *cfg_cmd, size_t cfg_cmd_len,
                              char *water_cmd, size_t water_cmd_len,
                              char *water_status, size_t water_status_len,
                              char *hard_reset, size_t hard_reset_len)
{
    mqtt_build_uuid();
    (void)snprintf(telemetry, telemetry_len, "devices/%s/telemetry", s_uuid);
    (void)snprintf(setup, setup_len, "devices/%s/setup", s_uuid);
    (void)snprintf(cfg_cmd, cfg_cmd_len, "devices/%s/config/cmd", s_uuid);
    (void)snprintf(water_cmd, water_cmd_len, "devices/%s/watering/cmd", s_uuid);
    (void)snprintf(water_status, water_status_len, "devices/%s/watering/status", s_uuid);

}

static void mqtt_gpio_init(void)
{
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << MQTT_WATER_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = false,
        .pull_up_en = false,
        .intr_type = GPIO_INTR_DISABLE,
    };
    (void)gpio_config(&io);
    gpio_set_level(MQTT_WATER_GPIO, 0);
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

static double mqtt_next_timestamp_ms(uint32_t unix_ts)
{
    double ts = 0.0;
    if (unix_ts > 0U) {
        ts = (double)unix_ts * 1000.0;
    }
    ts += (double)s_mqtt_msg_counter * 0.01;
    s_mqtt_msg_counter++;
    return ts;
}

static void mqtt_publish_watering_status(int water_on)
{
    char topic[MQTT_TOPIC_BUF_LEN] = {0};
    char dummy1[MQTT_TOPIC_BUF_LEN] = {0};
    char dummy2[MQTT_TOPIC_BUF_LEN] = {0};
    char dummy3[MQTT_TOPIC_BUF_LEN] = {0};
    char dummy4[MQTT_TOPIC_BUF_LEN] = {0};
    char dummy5[MQTT_TOPIC_BUF_LEN] = {0};
    mqtt_build_topics(dummy1, sizeof(dummy1), dummy2, sizeof(dummy2), dummy3, sizeof(dummy3), dummy4, sizeof(dummy4), topic, sizeof(topic), dummy5, sizeof(dummy5));

    char payload[64] = {0};
    (void)snprintf(payload, sizeof(payload), "{\"water\":%d}", water_on ? 1 : 0);
    (void)mqtt_publish_json(topic, payload, 1);
}

static void mqtt_apply_config(const cJSON *root)
{
    if (root == NULL) {
        return;
    }

    const cJSON *lux = cJSON_GetObjectItem(root, "lux");
    const cJSON *moi = cJSON_GetObjectItem(root, "moi");
    const cJSON *tem = cJSON_GetObjectItem(root, "tem");
    const cJSON *sle = cJSON_GetObjectItem(root, "sle");

    if (!cJSON_IsNumber(lux) || !cJSON_IsArray(moi) || !cJSON_IsArray(tem) || !cJSON_IsNumber(sle)) {
        ESP_LOGW(TAG, "config invalid");
        return;
    }

    if ((cJSON_GetArraySize(moi) != 4) || (cJSON_GetArraySize(tem) != 2)) {
        ESP_LOGW(TAG, "config invalid array sizes");
        return;
    }

    config_t cfg = {0};
    if (app_context_get_config(&cfg) != ESP_OK) {
        ESP_LOGW(TAG, "config read failed");
        return;
    }

    cfg.plant_config.lux = (uint8_t)cJSON_GetNumberValue(lux);
    for (int i = 0; i < 4; ++i) {
        const cJSON *item = cJSON_GetArrayItem(moi, i);
        if (!cJSON_IsNumber(item)) {
            ESP_LOGW(TAG, "config invalid moi");
            return;
        }
        cfg.plant_config.moi[i] = (uint8_t)cJSON_GetNumberValue(item);
    }

    for (int i = 0; i < 2; ++i) {
        const cJSON *item = cJSON_GetArrayItem(tem, i);
        if (!cJSON_IsNumber(item)) {
            ESP_LOGW(TAG, "config invalid tem");
            return;
        }
        cfg.plant_config.tem[i] = (uint16_t)cJSON_GetNumberValue(item);
    }

    cfg.plant_config.sleep_duration = (uint16_t)cJSON_GetNumberValue(sle);
    cfg.sleep_duration = cfg.plant_config.sleep_duration;

    if (app_context_set_config(&cfg) == ESP_OK) {
        ESP_LOGI(TAG, "config updated from mqtt");
    }
}

static void mqtt_handle_watering(const cJSON *root)
{
    if (root == NULL) {
        return;
    }

    const cJSON *dur = cJSON_GetObjectItem(root, "dur");
    if (!cJSON_IsNumber(dur)) {
        ESP_LOGW(TAG, "watering invalid");
        return;
    }

    int duration = (int)cJSON_GetNumberValue(dur);
    if (duration <= 0) {
        ESP_LOGW(TAG, "watering duration invalid");
        return;
    }

    gpio_set_level(MQTT_WATER_GPIO, 1);
    mqtt_publish_watering_status(1);
    vTaskDelay(pdMS_TO_TICKS((uint32_t)duration * 1000U));
    gpio_set_level(MQTT_WATER_GPIO, 0);
    mqtt_publish_watering_status(0);
}

static void mqtt_publish_telemetry_internal(void)
{
    sensor_data_t data = {0};
    (void)app_context_get_sensor_data(&data);

    char topic[MQTT_TOPIC_BUF_LEN] = {0};
    char setup_topic[MQTT_TOPIC_BUF_LEN] = {0};
    char cfg_topic[MQTT_TOPIC_BUF_LEN] = {0};
    char water_topic[MQTT_TOPIC_BUF_LEN] = {0};
    char water_status[MQTT_TOPIC_BUF_LEN] = {0};
    char hard_reset[MQTT_TOPIC_BUF_LEN] = {0};
    mqtt_build_topics(topic, sizeof(topic), setup_topic, sizeof(setup_topic), cfg_topic, sizeof(cfg_topic), water_topic, sizeof(water_topic), water_status, sizeof(water_status), hard_reset, sizeof(hard_reset));

    float temp_c = ((float)data.temperature / 10.0f) - 273.15f;

    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return;
    }

    uint32_t unix_ts = 0;
    if (app_context_is_time_synced()) {
        unix_ts = (uint32_t)time(NULL);
    }
    cJSON_AddNumberToObject(root, "timestamp", mqtt_next_timestamp_ms(unix_ts));

    cJSON *payload = cJSON_AddObjectToObject(root, "data");
    if (payload != NULL) {
        cJSON_AddNumberToObject(payload, "lux", (int)data.lux_level);
        cJSON_AddNumberToObject(payload, "tem", (double)temp_c);
        cJSON_AddNumberToObject(payload, "moi", (int)data.soil_moisture);
        cJSON_AddNumberToObject(payload, "pre", (double)data.pressure);
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

    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddNumberToObject(root, "timestamp", mqtt_next_timestamp_ms(timestamp));
    cJSON *payload = cJSON_AddObjectToObject(root, "data");
    if (payload != NULL) {
        cJSON_AddNumberToObject(payload, "lux", (int)data->lux_level);
        cJSON_AddNumberToObject(payload, "tem", (double)temp_c);
        cJSON_AddNumberToObject(payload, "moi", (int)data->soil_moisture);
        cJSON_AddNumberToObject(payload, "pre", (double)data->pressure);
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
    char setup_topic[MQTT_TOPIC_BUF_LEN] = {0};
    char cfg_topic[MQTT_TOPIC_BUF_LEN] = {0};
    char water_topic[MQTT_TOPIC_BUF_LEN] = {0};
    char water_status[MQTT_TOPIC_BUF_LEN] = {0};
    char hard_reset[MQTT_TOPIC_BUF_LEN] = {0};
    mqtt_build_topics(topic, sizeof(topic), setup_topic, sizeof(setup_topic), cfg_topic, sizeof(cfg_topic), water_topic, sizeof(water_topic), water_status, sizeof(water_status), hard_reset, sizeof(hard_reset));

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
    char water_topic[MQTT_TOPIC_BUF_LEN] = {0};
    char dummy1[MQTT_TOPIC_BUF_LEN] = {0};
    char dummy2[MQTT_TOPIC_BUF_LEN] = {0};
    char dummy3[MQTT_TOPIC_BUF_LEN] = {0};
    char dummy4[MQTT_TOPIC_BUF_LEN] = {0};
    mqtt_build_topics(dummy1, sizeof(dummy1), dummy2, sizeof(dummy2), cfg_topic, sizeof(cfg_topic), water_topic, sizeof(water_topic), dummy3, sizeof(dummy3), dummy4, sizeof(dummy4));

    if ((event->topic_len <= 0) || (event->data_len <= 0)) {
        return;
    }

    const bool is_cfg = (strlen(cfg_topic) == (size_t)event->topic_len) &&
                        (strncmp(event->topic, cfg_topic, event->topic_len) == 0);
    const bool is_water = (strlen(water_topic) == (size_t)event->topic_len) &&
                          (strncmp(event->topic, water_topic, event->topic_len) == 0);

    if (!is_cfg && !is_water) {
        return;
    }

    cJSON *root = cJSON_ParseWithLength(event->data, event->data_len);
    if (root == NULL) {
        ESP_LOGW(TAG, "json parse failed");
        return;
    }

    if (is_cfg) {
        mqtt_apply_config(root);
    } else if (is_water) {
        mqtt_handle_watering(root);
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
            s_mqtt_fail_count = 0;
            s_mqtt_fail_window_start_us = 0;

            mqtt_publish_stored_samples();

            if (!s_subscribed) {
                char cfg_topic[MQTT_TOPIC_BUF_LEN] = {0};
                char water_topic[MQTT_TOPIC_BUF_LEN] = {0};
                char dummy1[MQTT_TOPIC_BUF_LEN] = {0};
                char dummy2[MQTT_TOPIC_BUF_LEN] = {0};
                char dummy3[MQTT_TOPIC_BUF_LEN] = {0};
                char dummy4[MQTT_TOPIC_BUF_LEN] = {0};
                mqtt_build_topics(dummy1, sizeof(dummy1), dummy2, sizeof(dummy2), cfg_topic, sizeof(cfg_topic), water_topic, sizeof(water_topic), dummy3, sizeof(dummy3), dummy4, sizeof(dummy4));

                (void)esp_mqtt_client_subscribe(s_client, cfg_topic, 1);
                (void)esp_mqtt_client_subscribe(s_client, water_topic, 1);
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
            mqtt_track_failure_and_fallback();
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "mqtt disconnected");
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
    mqtt_gpio_init();

    char mqtt_uri[128] = {0};
    snprintf(mqtt_uri, sizeof(mqtt_uri), "mqtt://%s", MQTT_BROKER_HOST);

    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = mqtt_uri,
        .broker.address.port = MQTT_BROKER_PORT,
        .credentials.client_id = s_uuid,
        .credentials.username = s_uuid,
        .credentials.authentication.password = s_mqtt_pass,
        .session.keepalive = 60,
        .session.disable_clean_session = true,
    };

    s_client = esp_mqtt_client_init(&cfg);
    if (s_client == NULL) {
        return ESP_FAIL;
    }

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

    s_publish_pending = true;
   mqtt_publish_telemetry_internal();
    s_publish_pending = false;
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
    s_last_pub_id = -1;
    s_mqtt_fail_count = 0;
    s_mqtt_fail_window_start_us = 0;
    return ESP_OK;
}

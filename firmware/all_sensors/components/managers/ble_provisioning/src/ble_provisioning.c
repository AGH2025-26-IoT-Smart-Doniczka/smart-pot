#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_timer.h"
#include "esp_mac.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_common_api.h"
#include "esp_bt_defs.h"

#include "bsp_init.h"
#include "ssd1306.h"
#include "app_constants.h"
#include "app_context.h"
#include "app_events.h"
#include "fsm_manager.h"
#include "nvs_manager.h"
#include "ble_provisioning.h"

/* =========================================================================
   SECTION: Constants
   ========================================================================= */
#define ADV_DEVICE_NAME           "SmartPot"
#define PROV_TIMEOUT_US           (120ULL * 1000ULL * 1000ULL)
#define PROV_BIT_SSID             (1U << 0)
#define PROV_BIT_PASS             (1U << 1)
#define PROV_BIT_CFG              (1U << 2)
#define PROV_BIT_MQTT             (1U << 3)
#define PROV_EVENT_MASK           (PROV_BIT_SSID | PROV_BIT_PASS | PROV_BIT_CFG | PROV_BIT_MQTT)
#define PROV_MIN_CONFIG_LEN       11
#define PROV_MAX_CONFIG_LEN       12

/* =========================================================================
   SECTION: Attribute Table
   ========================================================================= */
static const uint16_t PRIMARY_SERVICE_UUID = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t CHAR_DECL_UUID = ESP_GATT_UUID_CHAR_DECLARE;
static const uint8_t CHAR_PROP_READ_WRITE = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE;
static const uint8_t CHAR_PROP_WRITE_ONLY = ESP_GATT_CHAR_PROP_BIT_WRITE;
// static const uint8_t CHAR_PROP_READ_ONLY = ESP_GATT_CHAR_PROP_BIT_READ;

static uint8_t SERVICE_UUID_128[16] = {
    0xa2, 0x90, 0x94, 0x47, 0x7a, 0x7f, 0xd8, 0xb8,
    0xd1, 0x40, 0x68, 0xa2, 0x37, 0xaa, 0x73, 0x5c
};
static const uint8_t CHAR_PASS_UUID_128[16] = {
    0xee, 0x0b, 0xfc, 0xd4, 0xbd, 0xce, 0x68, 0x98,
    0x0c, 0x4a, 0x72, 0x32, 0x1e, 0x3c, 0x6f, 0x45 
};
static const uint8_t CHAR_SSID_UUID_128[16] = {
    0x88, 0x97, 0x12, 0x43, 0x72, 0x82, 0x04, 0x9b,
    0xd1, 0x4a, 0xe9, 0x51, 0x05, 0x5f, 0xc3, 0xa3
};
static const uint8_t CHAR_CFG_UUID_128[16] = {
    0x29, 0x34, 0xe2, 0xce, 0x26, 0xf9, 0x47, 0x05,
    0xbd, 0x1d, 0xdf, 0xb3, 0x43, 0xf6, 0x3d, 0x04
};
static const uint8_t CHAR_MQTT_PASS_128[16] = {
    0x6c, 0x34, 0xa0, 0x9c, 0xfc, 0xd8, 0x54, 0x89,
    0x37, 0x4f, 0xa7, 0x9b, 0x57, 0xb6, 0xef, 0x5b
};


enum {
    IDX_SVC = 0,
    IDX_CHAR_SSID,
    IDX_CHAR_VAL_SSID,
    IDX_CHAR_PASS,
    IDX_CHAR_VAL_PASS,
    IDX_CHAR_CFG,
    IDX_CHAR_VAL_CFG,
    IDX_CHAR_MQTT_PASS,
    IDX_CHAR_VAL_MQTT_PASS,
    IDX_NB
};

/* =========================================================================
   SECTION: Static Data
   ========================================================================= */
static const char *TAG = "BLE_PROV";
static esp_gatt_if_t s_gatts_if = ESP_GATT_IF_NONE;
static uint16_t s_handle_table[IDX_NB];
static uint16_t s_conn_id = 0xFFFF;
static bool s_stack_ready;
static bool s_service_started;
static bool s_running;
static bool s_complete_sent;
static ssd1306_handle_t s_disp;
static EventGroupHandle_t s_bits;
static esp_timer_handle_t s_timeout_timer;
static esp_ble_adv_params_t s_adv_params = {
    .adv_int_min = 0x40,
    .adv_int_max = 0x60,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static uint8_t s_ssid_value[sizeof(((config_t *)0)->ssid)] = {0};
static uint8_t s_pass_value[sizeof(((config_t *)0)->passwd)] = {0};
static uint8_t s_cfg_value[PROV_MAX_CONFIG_LEN] = {0};
static uint8_t s_mqtt_pass_value[sizeof(((config_t *)0)->mqtt_passwd)] = {0};
static uint16_t s_mqtt_len = 0;
static uint16_t s_uid_len = 0;

/* =========================================================================
   SECTION: Attribute Declarations
   ========================================================================= */
static const esp_gatts_attr_db_t s_gatt_db[IDX_NB] = {
    [IDX_SVC] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&PRIMARY_SERVICE_UUID, ESP_GATT_PERM_READ,
            ESP_UUID_LEN_128, ESP_UUID_LEN_128, (uint8_t *)SERVICE_UUID_128}
    },
    [IDX_CHAR_SSID] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&CHAR_DECL_UUID, ESP_GATT_PERM_READ,
         sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&CHAR_PROP_READ_WRITE}
    },
    [IDX_CHAR_VAL_SSID] = {
        {ESP_GATT_RSP_BY_APP},
        {ESP_UUID_LEN_128, (uint8_t *)CHAR_SSID_UUID_128,
         ESP_GATT_PERM_WRITE_ENC_MITM,
         sizeof(s_ssid_value), 0, s_ssid_value}
    },
    [IDX_CHAR_PASS] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&CHAR_DECL_UUID, ESP_GATT_PERM_READ,
            sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&CHAR_PROP_WRITE_ONLY}
    },
    [IDX_CHAR_VAL_PASS] = {
        {ESP_GATT_RSP_BY_APP},
        {ESP_UUID_LEN_128, (uint8_t *)CHAR_PASS_UUID_128,
         ESP_GATT_PERM_WRITE_ENC_MITM,
         sizeof(s_pass_value), 0, s_pass_value}
    },
    [IDX_CHAR_CFG] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&CHAR_DECL_UUID, ESP_GATT_PERM_READ,
            sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&CHAR_PROP_WRITE_ONLY}
    },
    [IDX_CHAR_VAL_CFG] = {
        {ESP_GATT_RSP_BY_APP},
        {ESP_UUID_LEN_128, (uint8_t *)CHAR_CFG_UUID_128,
         ESP_GATT_PERM_WRITE_ENC_MITM,
         sizeof(s_cfg_value), 0, s_cfg_value}
    },
    [IDX_CHAR_MQTT_PASS] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&CHAR_DECL_UUID, ESP_GATT_PERM_READ,
            sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&CHAR_PROP_WRITE_ONLY}
    },
    [IDX_CHAR_VAL_MQTT_PASS] = {
        {ESP_GATT_RSP_BY_APP},
        {ESP_UUID_LEN_128, (uint8_t *)CHAR_MQTT_PASS_128,
         ESP_GATT_PERM_WRITE_ENC_MITM,
         sizeof(s_mqtt_pass_value), 0, s_mqtt_pass_value}
    },
};

/* =========================================================================
   SECTION: Forward Declarations
   ========================================================================= */
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
static void start_advertising(void);
static void apply_if_complete(void);
static void timeout_cb(void *arg);
static void display_show_passkey(uint32_t passkey);
static void display_clear(void);

/* =========================================================================
   SECTION: Helpers
   ========================================================================= */
static esp_err_t ensure_event_group(void)
{
    if (s_bits != NULL) {
        return ESP_OK;
    }
    s_bits = xEventGroupCreate();
    return (s_bits != NULL) ? ESP_OK : ESP_ERR_NO_MEM;
}

static void reset_session_state(void)
{
    (void)memset(s_ssid_value, 0, sizeof(s_ssid_value));
    (void)memset(s_pass_value, 0, sizeof(s_pass_value));
    (void)memset(s_cfg_value, 0, sizeof(s_cfg_value));
    (void)memset(s_mqtt_pass_value, 0, sizeof(s_mqtt_pass_value));
    s_mqtt_len = 0;
    s_complete_sent = false;
    if (s_bits) {
        (void)xEventGroupClearBits(s_bits, PROV_EVENT_MASK);
    }
}

static esp_err_t ensure_stack_ready(void)
{
    if (s_stack_ready) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT), TAG, "release classic");

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_bt_controller_init(&bt_cfg), TAG, "bt ctrl init");
    ESP_RETURN_ON_ERROR(esp_bt_controller_enable(ESP_BT_MODE_BLE), TAG, "bt ctrl en");
    ESP_RETURN_ON_ERROR(esp_bluedroid_init(), TAG, "bluedroid init");
    ESP_RETURN_ON_ERROR(esp_bluedroid_enable(), TAG, "bluedroid en");

    ESP_RETURN_ON_ERROR(esp_ble_gap_register_callback(gap_event_handler), TAG, "gap cb");
    ESP_RETURN_ON_ERROR(esp_ble_gatts_register_callback(gatts_event_handler), TAG, "gatts cb");
    ESP_RETURN_ON_ERROR(esp_ble_gatts_app_register(0), TAG, "gatts app");

    uint8_t auth = ESP_LE_AUTH_REQ_SC_MITM_BOND;
    uint8_t iocap = ESP_IO_CAP_OUT;
    uint8_t key_size = 16;
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;

    (void)esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth, sizeof(auth));
    (void)esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(iocap));
    (void)esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(key_size));
    (void)esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(init_key));
    (void)esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(rsp_key));

    s_stack_ready = true;
    ESP_LOGI(TAG, "BLE stack ready");
    return ESP_OK;
}

static void display_show_passkey(uint32_t passkey)
{
    if (s_disp == NULL) {
        s_disp = app_context_get_display_handle();
        if (s_disp == NULL) {
            return;
        }
    }

    char pass_buf[8] = {0};
    (void)snprintf(pass_buf, sizeof(pass_buf), "%06" PRIu32, passkey);

    (void)ssd1306_clear(s_disp);
    (void)ssd1306_draw_text(s_disp, "PASSKEY", 0, 0);
    (void)ssd1306_draw_text(s_disp, pass_buf, 0, 2);
    (void)ssd1306_flush(s_disp);
}

static void display_clear(void)
{
    if (s_disp == NULL) {
        s_disp = app_context_get_display_handle();
        if (s_disp == NULL) {
            return;
        }
    }

    (void)ssd1306_clear(s_disp);
    (void)ssd1306_flush(s_disp);
}


static void start_timeout_timer(void)
{
    if (s_timeout_timer != NULL) {
        return;
    }

    const esp_timer_create_args_t args = {
        .callback = timeout_cb,
        .name = "prov_timeout",
    };

    if (esp_timer_create(&args, &s_timeout_timer) != ESP_OK) {
        return;
    }
    (void)esp_timer_start_once(s_timeout_timer, PROV_TIMEOUT_US);
}

static void timeout_cb(void *arg)
{
    (void)arg;
    s_running = false;
    (void)fsm_manager_post_event(APP_EVENT_PROV_TIMEOUT, NULL, 0, 0);
}

static void stop_timeout_timer(void)
{
    if (s_timeout_timer) {
        (void)esp_timer_stop(s_timeout_timer);
        (void)esp_timer_delete(s_timeout_timer);
        s_timeout_timer = NULL;
    }
}

static void send_write_response(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, esp_gatt_status_t status)
{
    if (!param->write.need_rsp) {
        return;
    }

    esp_gatt_rsp_t rsp = {0};
    rsp.attr_value.handle = param->write.handle;
    rsp.attr_value.len = 0;
    rsp.attr_value.auth_req = ESP_GATT_AUTH_REQ_MITM;
    (void)esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, &rsp);
}


static void start_advertising(void)
{
    if (!s_running || !s_service_started) {
        return;
    }
    esp_ble_adv_data_t adv = {
        .set_scan_rsp = false,
        .include_name = true,
        .include_txpower = false,
        .min_interval = 0x20,
        .max_interval = 0x40,
        .appearance = 0,
        .service_uuid_len = sizeof(SERVICE_UUID_128),
        .p_service_uuid = SERVICE_UUID_128,
        .flag = ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT,
    };
    (void)esp_ble_gap_config_adv_data(&adv);
}

static void apply_if_complete(void)
{
    if (!s_running || s_complete_sent || s_bits == NULL) {
        return;
    }

    EventBits_t bits = xEventGroupGetBits(s_bits);
    if ((bits & PROV_EVENT_MASK) != PROV_EVENT_MASK) {
        return;
    }

    config_t cfg = {0};
    if (app_context_get_config(&cfg) != ESP_OK) {
        memset(&cfg, 0, sizeof(cfg));
    }

    size_t ssid_len = strnlen((const char *)s_ssid_value, sizeof(s_ssid_value));
    size_t pass_len = strnlen((const char *)s_pass_value, sizeof(s_pass_value));
    if (ssid_len > sizeof(cfg.ssid) - 1U || pass_len > sizeof(cfg.passwd) - 1U) {
        ESP_LOGW(TAG, "credentials too long (ssid=%lu pass=%lu)", (unsigned long)ssid_len, (unsigned long)pass_len);
        return;
    }

    memset(cfg.ssid, 0, sizeof(cfg.ssid));
    memset(cfg.passwd, 0, sizeof(cfg.passwd));
    memcpy(cfg.ssid, s_ssid_value, ssid_len);
    memcpy(cfg.passwd, s_pass_value, pass_len);

    ESP_LOGI(TAG, "prov ssid='%s' wifi_pass='%s'", cfg.ssid, cfg.passwd);

    if (s_mqtt_len > sizeof(cfg.mqtt_passwd)) {
        ESP_LOGW(TAG, "mqtt password too long (%u)", (unsigned)s_mqtt_len);
        return;
    }
    memset(cfg.mqtt_passwd, 0, sizeof(cfg.mqtt_passwd));
    if (s_mqtt_len > 0U) {
        memcpy(cfg.mqtt_passwd, s_mqtt_pass_value, s_mqtt_len);
    }

    char mqtt_hex[(sizeof(cfg.mqtt_passwd) * 3U) + 1U] = {0};
    size_t off = 0;
    for (size_t i = 0; i < s_mqtt_len && i < sizeof(cfg.mqtt_passwd); ++i) {
        off += (size_t)snprintf(&mqtt_hex[off], sizeof(mqtt_hex) - off, "%02X ", cfg.mqtt_passwd[i]);
    }
    char mqtt_str[sizeof(cfg.mqtt_passwd) + 1U] = {0};
    if (s_mqtt_len > 0U) {
        size_t copy_len = s_mqtt_len;
        if (copy_len > sizeof(cfg.mqtt_passwd)) {
            copy_len = sizeof(cfg.mqtt_passwd);
        }
        memcpy(mqtt_str, cfg.mqtt_passwd, copy_len);
        mqtt_str[copy_len] = '\0';
    }
    ESP_LOGI(TAG, "prov mqtt_pass_len=%u mqtt_pass='%s' mqtt_pass_hex=%s", (unsigned)s_mqtt_len, mqtt_str, mqtt_hex);

    memcpy(&cfg.plant_config, s_cfg_value, sizeof(cfg.plant_config));
    cfg.sleep_duration = cfg.plant_config.sleep_duration;

    (void)app_context_set_config(&cfg);
    esp_err_t err = nvs_manager_save_config(&cfg);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "save cfg failed (%s)", esp_err_to_name(err));
        return;
    }

    s_complete_sent = true;
    stop_timeout_timer();
    display_clear();
    ESP_LOGI(TAG, "provisioning complete (ssid_len=%lu)", (unsigned long)ssid_len);
    (void)fsm_manager_post_event(APP_EVENT_PROV_DATA_RECVD, NULL, 0, 0);
}

/* =========================================================================
   SECTION: GAP
   ========================================================================= */
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            if (s_running) {
                (void)esp_ble_gap_start_advertising(&s_adv_params);
            }
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "adv start failed");
            }
            break;
        case ESP_GAP_BLE_SEC_REQ_EVT:
            (void)esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
            break;
        case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:
            ESP_LOGI(TAG, "passkey: %06" PRIu32, param->ble_security.key_notif.passkey);
            display_show_passkey(param->ble_security.key_notif.passkey);
            
            break;
        case ESP_GAP_BLE_AUTH_CMPL_EVT:
            esp_bd_addr_t bd_addr;
            memcpy(bd_addr, param->ble_security.auth_cmpl.bd_addr, sizeof(esp_bd_addr_t));
            ESP_LOGI(TAG, "remote BD_ADDR: %08x%04x",
                    (bd_addr[0] << 24) + (bd_addr[1] << 16) + (bd_addr[2] << 8) + bd_addr[3],
                    (bd_addr[4] << 8) + bd_addr[5]);
            ESP_LOGI(TAG, "address type = %d", param->ble_security.auth_cmpl.addr_type);
            ESP_LOGI(TAG, "pair status = %s",param->ble_security.auth_cmpl.success ? "success" : "fail");
            if (!param->ble_security.auth_cmpl.success) {
                ESP_LOGI(TAG, "fail reason = 0x%x",param->ble_security.auth_cmpl.fail_reason);
            } else {
                ESP_LOGI(TAG, "auth mode = %s",param->ble_security.auth_cmpl.auth_mode ? "ESP_LE_AUTH_REQ_SC_MITM_BOND" : "ESP_LE_AUTH_REQ_SC_ONLY");
                display_clear();
                if (s_disp != NULL) {
                    ssd1306_draw_text(s_disp, "PAIRED!", 0, 2);
                    ssd1306_flush(s_disp);
                }
            }

            break;
        default:
            break;
    }
}

/* =========================================================================
   SECTION: GATTS
   ========================================================================= */
static void handle_write_evt(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    uint16_t handle = param->write.handle;
    uint16_t len = param->write.len;
    const uint8_t *value = param->write.value;

    ESP_LOGI(TAG, "write handle=0x%04X len=%u (ssid=%u pass=%u cfg=%u mqtt=%u)",
             (unsigned)handle,
             (unsigned)len,
             (unsigned)sizeof(s_ssid_value),
             (unsigned)sizeof(s_pass_value),
             (unsigned)sizeof(s_cfg_value),
             (unsigned)sizeof(s_mqtt_pass_value));

    if (!s_running) {
        send_write_response(gatts_if, param, ESP_GATT_OK);
        return;
    }

    if (handle == s_handle_table[IDX_CHAR_VAL_SSID]) {
        size_t to_copy = (len < sizeof(s_ssid_value)) ? len : sizeof(s_ssid_value) - 1U;
        memset(s_ssid_value, 0, sizeof(s_ssid_value));
        memcpy(s_ssid_value, value, to_copy);
        (void)esp_ble_gatts_set_attr_value(handle, (uint16_t)to_copy, s_ssid_value);
        (void)xEventGroupSetBits(s_bits, PROV_BIT_SSID);
        ESP_LOGI(TAG, "ssid write len=%u", (unsigned)to_copy);
        apply_if_complete();
        send_write_response(gatts_if, param, ESP_GATT_OK);
        return;
    }
    if (handle == s_handle_table[IDX_CHAR_VAL_PASS]) {
        size_t to_copy = (len < sizeof(s_pass_value)) ? len : sizeof(s_pass_value) - 1U;
        memset(s_pass_value, 0, sizeof(s_pass_value));
        memcpy(s_pass_value, value, to_copy);
        (void)esp_ble_gatts_set_attr_value(handle, (uint16_t)to_copy, s_pass_value);
        (void)xEventGroupSetBits(s_bits, PROV_BIT_PASS);
        ESP_LOGI(TAG, "pass write len=%u", (unsigned)to_copy);
        apply_if_complete();
        send_write_response(gatts_if, param, ESP_GATT_OK);
        return;
    }

    if (handle == s_handle_table[IDX_CHAR_VAL_CFG]) {
        if (len < PROV_MIN_CONFIG_LEN || len > PROV_MAX_CONFIG_LEN) {
            ESP_LOGW(TAG, "cfg len invalid (%u)", (unsigned)len);
            send_write_response(gatts_if, param, ESP_GATT_INVALID_ATTR_LEN);
            return;
        }
        memset(s_cfg_value, 0, sizeof(s_cfg_value));
        memcpy(s_cfg_value, value, len);
        char cfg_hex[(3U * PROV_MAX_CONFIG_LEN) + 1U] = {0};
        size_t off = 0;
        for (size_t i = 0; i < PROV_MAX_CONFIG_LEN; ++i) {
            off += (size_t)snprintf(&cfg_hex[off], sizeof(cfg_hex) - off, "%02X ", s_cfg_value[i]);
        }
        ESP_LOGI(TAG, "cfg bytes: %s", cfg_hex);

        (void)esp_ble_gatts_set_attr_value(handle, PROV_MAX_CONFIG_LEN, s_cfg_value);
        (void)xEventGroupSetBits(s_bits, PROV_BIT_CFG);
        ESP_LOGI(TAG, "config write ok");
        apply_if_complete();
        send_write_response(gatts_if, param, ESP_GATT_OK);
        return;
    }

    if (handle == s_handle_table[IDX_CHAR_VAL_MQTT_PASS]) {
        if (len == 0U) {
            ESP_LOGW(TAG, "mqtt pass empty");
            send_write_response(gatts_if, param, ESP_GATT_INVALID_ATTR_LEN);
            return;
        }
        size_t to_copy = (len < sizeof(s_mqtt_pass_value)) ? len : sizeof(s_mqtt_pass_value);
        memset(s_mqtt_pass_value, 0, sizeof(s_mqtt_pass_value));
        memcpy(s_mqtt_pass_value, value, to_copy);
        s_mqtt_len = (uint16_t)to_copy;
        (void)esp_ble_gatts_set_attr_value(handle, (uint16_t)to_copy, s_mqtt_pass_value);
        (void)xEventGroupSetBits(s_bits, PROV_BIT_MQTT);
        ESP_LOGI(TAG, "mqtt pass write len=%u", (unsigned)to_copy);
        apply_if_complete();
        send_write_response(gatts_if, param, ESP_GATT_OK);
        return;
    }

    send_write_response(gatts_if, param, ESP_GATT_OK);
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    if (event == ESP_GATTS_REG_EVT) {
        s_gatts_if = gatts_if;
    }

    switch (event) {
        case ESP_GATTS_REG_EVT:
            (void)esp_ble_gap_set_device_name(ADV_DEVICE_NAME);
            (void)esp_ble_gatts_create_attr_tab(s_gatt_db, gatts_if, IDX_NB, 0);
            (void)esp_ble_gatt_set_local_mtu(200);
            break;
        case ESP_GATTS_CREAT_ATTR_TAB_EVT:
            if (param->add_attr_tab.status != ESP_GATT_OK) {
                ESP_LOGE(TAG, "attr table create failed");
                break;
            }
            memcpy(s_handle_table, param->add_attr_tab.handles, sizeof(s_handle_table));
            (void)esp_ble_gatts_start_service(s_handle_table[IDX_SVC]);
            s_service_started = true;
            start_advertising();
            break;
        case ESP_GATTS_START_EVT:
            s_service_started = true;
            start_advertising();
            break;
        case ESP_GATTS_CONNECT_EVT:
            s_conn_id = param->connect.conn_id;
            (void)esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_MITM);
            (void)fsm_manager_post_event(APP_EVENT_PROV_CONNECTED, NULL, 0, 0);
            break;
        case ESP_GATTS_DISCONNECT_EVT:
            s_conn_id = 0xFFFF;
            if (s_running) {
                start_advertising();
            }
            break;
        case ESP_GATTS_WRITE_EVT:
            handle_write_evt(gatts_if, param);
            break;
        default:
            break;
    }
}

/* =========================================================================
   SECTION: Public API
   ========================================================================= */
esp_err_t ble_provisioning_init(void)
{
    ESP_RETURN_ON_ERROR(ensure_event_group(), TAG, "event group");
    ESP_RETURN_ON_ERROR(ensure_stack_ready(), TAG, "stack");
    return ESP_OK;
}

esp_err_t ble_provisioning_start(void)
{
    ESP_RETURN_ON_ERROR(ble_provisioning_init(), TAG, "init");

    reset_session_state();
    s_running = true;
    s_disp = app_context_ensure_display();
    start_timeout_timer();
    if (s_service_started) {
        start_advertising();
    }
    ESP_LOGI(TAG, "provisioning started");
    return ESP_OK;
}

void ble_provisioning_stop(void)
{
    s_running = false;
    stop_timeout_timer();
    if (s_service_started) {
        (void)esp_ble_gap_stop_advertising();
    }
    display_clear();
    reset_session_state();
    ESP_LOGI(TAG, "provisioning stopped");
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdatomic.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_random.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "sdkconfig.h"
#include "ssd1306.h"
#include "i2c.h"
#include "ble_server.h"

#define GATTS_TAG "GATTS_SERVER"
#define NVS_NAMESPACE "wifi_prov"

static i2c_master_dev_handle_t oled_handle = NULL;
static i2c_master_bus_handle_t bus_handle = NULL;
static bool wifi_provisioned = false;
static atomic_bool device_connected = false;

#define PROFILE_NUM                 1
#define PROFILE_APP_ID              0

#define DEVICE_NAME          "Smart Pot"

// Custom 128-bit UUIDs
//a2909447-7a7f-d8b8-d140-68a237aa735c
static const uint8_t SERVICE_UUID[16] = {
    0xa2, 0x90, 0x94, 0x47, 0x7a, 0x7f, 0xd8, 0xb8,
    0xd1, 0x40, 0x68, 0xa2, 0x37, 0xaa, 0x73, 0x5c
};

//ee0bfcd4-bdce-6898-0c4a-72321e3c6f45
static const uint8_t CHAR_WIFI_PASSWD_UUID[16] = {
    0xee, 0x0b, 0xfc, 0xd4, 0xbd, 0xce, 0x68, 0x98,
    0x0c, 0x4a, 0x72, 0x32, 0x1e, 0x3c, 0x6f, 0x45
};

//88971243-7282-049b-d14a-e951055fc3a3
static const uint8_t CHAR_WIFI_SSID_UUID[16] = {
    0x88, 0x97, 0x12, 0x43, 0x72, 0x82, 0x04, 0x9b,
    0xd1, 0x4a, 0xe9, 0x51, 0x05, 0x5f, 0xc3, 0xa3
};

//2934e2ce-26f9-4705-bd1d-dfb343f63d04
static const uint8_t CHAR_CONFIG_UUID[16] = {
    0x29, 0x34, 0xe2, 0xce, 0x26, 0xf9, 0x47, 0x05, 
    0xbd, 0x1d, 0xdf, 0xb3, 0x43, 0xf6, 0x3d, 0x04
};

//752ff574-058c-4ba3-8310-b6daa639ee4d
static const uint8_t CHAR_MAC_UUID[16] = {
    0x75, 0x2f, 0xf5, 0x74, 0x05, 0x8c, 0x4b, 0xa3, 
    0x83, 0x10, 0xb6, 0xda, 0xa6, 0x39, 0xee, 0x4d
};

static const uint8_t CHAR_MQTT_PASSWD_UUID[16] = {
    0x5b, 0xef, 0xb6, 0x57, 0x9b, 0xa7, 0x4f, 0x37, 
    0x89, 0x54, 0xd8, 0xfc, 0x9c, 0xa0, 0x34, 0x6c
};





// GATT declarations
static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t char_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;

static const uint8_t char_prop_read_write = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE;

// Characteristic values storage
static uint8_t wifi_passwd_value[64] = {0};
static uint8_t wifi_ssid_value[32] = {0};

// Attribute table indices
enum {
    IDX_SVC,
    IDX_CHAR_WIFI_PASSWD,
    IDX_CHAR_VAL_WIFI_PASSWD,
    IDX_CHAR_WIFI_SSID,
    IDX_CHAR_VAL_WIFI_SSID,
    IDX_NB,
};

static uint16_t prov_handle_table[IDX_NB];

// GATT Service Table - MITM protected
static const esp_gatts_attr_db_t prov_gatt_db[IDX_NB] = {
    // Service Declaration
    [IDX_SVC] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
         ESP_UUID_LEN_128, ESP_UUID_LEN_128, (uint8_t *)SERVICE_UUID}
    },
    // WiFi Password Characteristic Declaration
    [IDX_CHAR_WIFI_PASSWD] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&char_declaration_uuid, ESP_GATT_PERM_READ,
         sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&char_prop_read_write}
    },
    // WiFi Password Characteristic Value - MITM protected
    [IDX_CHAR_VAL_WIFI_PASSWD] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_128, (uint8_t *)CHAR_WIFI_PASSWD_UUID,
         ESP_GATT_PERM_WRITE_ENC_MITM,
         sizeof(wifi_passwd_value), 0, wifi_passwd_value}
    },
    // WiFi SSID Characteristic Declaration
    [IDX_CHAR_WIFI_SSID] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&char_declaration_uuid, ESP_GATT_PERM_READ,
         sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&char_prop_read_write}
    },
    // WiFi SSID Characteristic Value - MITM protected
    [IDX_CHAR_VAL_WIFI_SSID] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_128, (uint8_t *)CHAR_WIFI_SSID_UUID,
         ESP_GATT_PERM_READ_ENC_MITM | ESP_GATT_PERM_WRITE_ENC_MITM,
         sizeof(wifi_ssid_value), 0, wifi_ssid_value}
    },
};

static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp        = false,
    .include_name        = true,
    .include_txpower     = true,
    .min_interval        = 0x0006, 
    .max_interval        = 0x0010, 
    .appearance          = 0x0341, 
    .manufacturer_len    = 0,
    .p_manufacturer_data = NULL,
    .service_data_len    = 0,
    .p_service_data      = NULL,
    .service_uuid_len    = 0,
    .p_service_uuid      = NULL,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min         = 0x20,
    .adv_int_max         = 0x40,
    .adv_type            = ADV_TYPE_IND,
    .own_addr_type       = BLE_ADDR_TYPE_PUBLIC,
    .channel_map         = ADV_CHNL_ALL,
    .adv_filter_policy   = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
};

static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

static struct gatts_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_APP_ID] = {
        .gatts_cb = gatts_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,
    },
};

bool ble_is_wifi_provisioned(void) {
    nvs_handle_t nvs;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs) != ESP_OK) return false;
    
    size_t len;
    bool has_ssid = (nvs_get_str(nvs, "ssid", NULL, &len) == ESP_OK && len > 1);
    bool has_pass = (nvs_get_str(nvs, "password", NULL, &len) == ESP_OK && len > 1);
    
    if (has_ssid && has_pass) {
        char ssid[33] = {0};
        char pass[65] = {0};
        size_t ssid_len = sizeof(ssid);
        size_t pass_len = sizeof(pass);
        nvs_get_str(nvs, "ssid", ssid, &ssid_len);
        nvs_get_str(nvs, "password", pass, &pass_len);
        ESP_LOGI(GATTS_TAG, "WiFi credentials found - SSID: %s, Password: %s", ssid, pass);
    }
    
    nvs_close(nvs);
    
    return has_ssid && has_pass;
}

bool ble_is_connected(void) {
    return atomic_load(&device_connected);
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        if (!wifi_provisioned) {
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(GATTS_TAG, "Advertising start failed");
        }
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
        ESP_LOGI(GATTS_TAG, "Connection params update, status %d, conn_int %d, latency %d, timeout %d",
                  param->update_conn_params.status,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
        break;
    case ESP_GAP_BLE_SEC_REQ_EVT:
        /* send the positive(true) security response to the peer device to accept the security request.
        If not accept the security request, should send the security response with negative(false).*/
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;
    case ESP_GAP_BLE_PASSKEY_NOTIF_EVT: {
        ESP_LOGI(GATTS_TAG, "ESP_GAP_BLE_PASSKEY_NOTIF_EVT passkey = %06" PRIu32, param->ble_security.key_notif.passkey);
        char passkey_str[10];
        snprintf(passkey_str, sizeof(passkey_str), "%06" PRIu32, param->ble_security.key_notif.passkey);
        if (oled_handle) {
            ssd1306_clear_buffer();
            ssd1306_draw_text("PASSKEY:", 0, 0);
            ssd1306_draw_text(passkey_str, 0, 2);
            ssd1306_update_display(oled_handle);
        }
        break;
    }
    case ESP_GAP_BLE_AUTH_CMPL_EVT: {
        esp_bd_addr_t bd_addr;
        memcpy(bd_addr, param->ble_security.auth_cmpl.bd_addr, sizeof(esp_bd_addr_t));
        ESP_LOGI(GATTS_TAG, "remote BD_ADDR: %08x%04x",
                (bd_addr[0] << 24) + (bd_addr[1] << 16) + (bd_addr[2] << 8) + bd_addr[3],
                (bd_addr[4] << 8) + bd_addr[5]);
        ESP_LOGI(GATTS_TAG, "address type = %d", param->ble_security.auth_cmpl.addr_type);
        ESP_LOGI(GATTS_TAG, "pair status = %s",param->ble_security.auth_cmpl.success ? "success" : "fail");
        if (!param->ble_security.auth_cmpl.success) {
            ESP_LOGI(GATTS_TAG, "fail reason = 0x%x",param->ble_security.auth_cmpl.fail_reason);
        } else {
            ESP_LOGI(GATTS_TAG, "auth mode = %s",param->ble_security.auth_cmpl.auth_mode ? "ESP_LE_AUTH_REQ_SC_MITM_BOND" : "ESP_LE_AUTH_REQ_SC_ONLY");
            if (oled_handle) {
                ssd1306_clear_buffer();
                ssd1306_draw_text("PAIRED!", 0, 2);
                ssd1306_update_display(oled_handle);
            }
        }
        break;
    }
    default:
        break;
    }
}

static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
    case ESP_GATTS_REG_EVT: {
        esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(DEVICE_NAME);
        if (set_dev_name_ret) {
            ESP_LOGE(GATTS_TAG, "set device name failed, error code = %x", set_dev_name_ret);
        }
        esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
        if (ret) {
            ESP_LOGE(GATTS_TAG, "config adv data failed, error code = %x", ret);
        }
        esp_ble_gatts_create_attr_tab(prov_gatt_db, gatts_if, IDX_NB, 0);
        break;
    }
    case ESP_GATTS_READ_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_READ_EVT, handle %d", param->read.handle);
        break;
        
    case ESP_GATTS_WRITE_EVT: {
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_WRITE_EVT, handle %d, len %d", param->write.handle, param->write.len);
        
        nvs_handle_t nvs;
        esp_err_t err;
        
        if (param->write.handle == prov_handle_table[IDX_CHAR_VAL_WIFI_SSID]) {
            memcpy(wifi_ssid_value, param->write.value, param->write.len);
            wifi_ssid_value[param->write.len] = '\0';
            ESP_LOGI(GATTS_TAG, "WiFi SSID received: %s", (char *)wifi_ssid_value);
            
            err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs);
            if (err == ESP_OK) {
                nvs_set_str(nvs, "ssid", (char *)wifi_ssid_value);
                nvs_commit(nvs);
                nvs_close(nvs);
                ESP_LOGI(GATTS_TAG, "SSID saved to NVS");
            }
            
            ssd1306_draw_text("SSID:   ", 0, 0);
            ssd1306_draw_text((char *)wifi_ssid_value, 0, 1);
            ssd1306_update_display(oled_handle);
        } 
        else if (param->write.handle == prov_handle_table[IDX_CHAR_VAL_WIFI_PASSWD]) {
            memcpy(wifi_passwd_value, param->write.value, param->write.len);
            wifi_passwd_value[param->write.len] = '\0';
            ESP_LOGI(GATTS_TAG, "WiFi Password received: %s", (char *)wifi_passwd_value);
            
            err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs);
            if (err == ESP_OK) {
                nvs_set_str(nvs, "password", (char *)wifi_passwd_value);
                nvs_commit(nvs);
                nvs_close(nvs);
                ESP_LOGI(GATTS_TAG, "Password saved to NVS");
            }
            
            ssd1306_draw_text("Hasło:   ", 0, 2);
            ssd1306_draw_text((char *)wifi_passwd_value, 0, 3);
            ssd1306_update_display(oled_handle);
        }
        
        if (ble_is_wifi_provisioned()) {
            wifi_provisioned = true;
            esp_ble_gap_stop_advertising();
            ESP_LOGI(GATTS_TAG, "WiFi provisioned, stopping BLE advertising");
        }
        break;
    }

    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
        break;
    case ESP_GATTS_CONF_EVT:
        break;
    
    case ESP_GATTS_CREAT_ATTR_TAB_EVT:
        if (param->add_attr_tab.status == ESP_GATT_OK) {
            if (param->add_attr_tab.num_handle == IDX_NB) {
                ESP_LOGI(GATTS_TAG, "Provisioning service created, handles: %d", param->add_attr_tab.num_handle);
                memcpy(prov_handle_table, param->add_attr_tab.handles, sizeof(prov_handle_table));
                esp_ble_gatts_start_service(prov_handle_table[IDX_SVC]);
            }
        } else {
            ESP_LOGE(GATTS_TAG, "Create attr table failed, error code = %x", param->add_attr_tab.status);
        }
        break;
    case ESP_GATTS_CONNECT_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONNECT_EVT, conn_id %d", param->connect.conn_id);
        gl_profile_tab[PROFILE_APP_ID].conn_id = param->connect.conn_id;
        atomic_store(&device_connected, true);
        
        esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_MITM);
        
        esp_ble_conn_update_params_t conn_params = {0};
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        conn_params.latency = 0;
        conn_params.max_int = 0x20;    
        conn_params.min_int = 0x10;    
        conn_params.timeout = 400;    
        esp_ble_gap_update_conn_params(&conn_params);
        break;

    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_DISCONNECT_EVT");
        gl_profile_tab[PROFILE_APP_ID].conn_id = 0;
        atomic_store(&device_connected, false);
        if (!wifi_provisioned) {
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    default:
        break;
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gl_profile_tab[param->reg.app_id].gatts_if = gatts_if;
        } else {
            ESP_LOGI(GATTS_TAG, "Reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }

    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            if (gatts_if == ESP_GATT_IF_NONE || 
                    gatts_if == gl_profile_tab[idx].gatts_if) {
                if (gl_profile_tab[idx].gatts_cb) {
                    gl_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}

void ble_server_start(i2c_master_bus_handle_t i2c_bus, i2c_master_dev_handle_t oled)
{
    bus_handle = i2c_bus;
    oled_handle = oled;
    esp_err_t ret;
    
    wifi_provisioned = ble_is_wifi_provisioned();
    if (wifi_provisioned) {
        ESP_LOGI(GATTS_TAG, "WiFi already provisioned, BLE will not advertise");
    }

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s initialize controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret) {
        ESP_LOGE(GATTS_TAG, "gatts register error, error code = %x", ret);
        return;
    }

    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret) {
        ESP_LOGE(GATTS_TAG, "gap register error, error code = %x", ret);
        return;
    }

    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;    
    esp_ble_io_cap_t iocap = ESP_IO_CAP_OUT;        
    uint8_t key_size = 16;     
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));

    ret = esp_ble_gatts_app_register(PROFILE_APP_ID);
    if (ret) {
        ESP_LOGE(GATTS_TAG, "gatts app register error, error code = %x", ret);
        return;
    }
    
    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret) {
        ESP_LOGE(GATTS_TAG, "set local MTU failed, error code = %x", local_mtu_ret);
    }
}

void ble_remove_all_bonds(void) {
    int dev_num = esp_ble_get_bond_device_num();
    if (dev_num == 0) {
        ESP_LOGI(GATTS_TAG, "No bonded devices");
        return;
    }
    
    esp_ble_bond_dev_t *dev_list = malloc(sizeof(esp_ble_bond_dev_t) * dev_num);
    if (dev_list == NULL) {
        ESP_LOGE(GATTS_TAG, "Failed to allocate memory for bond list");
        return;
    }
    
    esp_ble_get_bond_device_list(&dev_num, dev_list);
    
    for (int i = 0; i < dev_num; i++) {
        esp_ble_remove_bond_device(dev_list[i].bd_addr);
        ESP_LOGI(GATTS_TAG, "Removed bond: "ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(dev_list[i].bd_addr));
    }
    
    free(dev_list);
    ESP_LOGI(GATTS_TAG, "Removed %d bonded devices", dev_num);
}

void ble_server_stop(void) {
    esp_ble_gap_stop_advertising();
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
    ESP_LOGI(GATTS_TAG, "BLE server stopped");
}

static void erase_wifi_credentials(void) {
    nvs_handle_t nvs;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_erase_key(nvs, "ssid");
        nvs_erase_key(nvs, "password");
        nvs_commit(nvs);
        nvs_close(nvs);
        ESP_LOGI(GATTS_TAG, "WiFi credentials erased");
    }
    
    memset(wifi_ssid_value, 0, sizeof(wifi_ssid_value));
    memset(wifi_passwd_value, 0, sizeof(wifi_passwd_value));
    wifi_provisioned = false;
}

void ble_factory_reset(void) {
    ble_remove_all_bonds();
    erase_wifi_credentials();
    
    esp_ble_gap_start_advertising(&adv_params);
    ESP_LOGI(GATTS_TAG, "Factory reset complete, advertising started");
}

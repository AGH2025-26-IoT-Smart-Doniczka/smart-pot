#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_check.h"
#include "nvs_manager.h"

static const char *TAG = "NVS_MGR";

/* =========================================================================
   SECTION: Constants
   ========================================================================= */
#define NVS_NAMESPACE        "app"
#define NVS_KEY_CONFIG       "cfg"
#define NVS_KEY_CONFIG_SET   "cfg_set"
#define NVS_KEY_META_NEXT    "meta_next"
#define NVS_KEY_META_COUNT   "meta_cnt"

/* =========================================================================
   SECTION: Static State
   ========================================================================= */
static nvs_handle_t s_nvs = 0;
static bool s_ready = false;

/* =========================================================================
   SECTION: Helpers
   ========================================================================= */
static esp_err_t ensure_nvs(void)
{
    if (s_ready) {
        return ESP_OK;
    }

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_flash_init failed (%s)", esp_err_to_name(err));
        return err;
    }

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &s_nvs);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open failed (%s)", esp_err_to_name(err));
        return err;
    }

    s_ready = true;
    return ESP_OK;
}

static bool config_is_valid(const config_t *cfg)
{
    if (cfg == NULL) {
        return false;
    }

    size_t ssid_len = strnlen(cfg->ssid, sizeof(cfg->ssid));
    if (ssid_len == 0 || ssid_len > MAX_SSID_LENGTH) {
        ESP_LOGW(TAG, "config invalid: ssid_len=%lu", (unsigned long)ssid_len);
        return false;
    }

    size_t pwd_len = strnlen(cfg->passwd, sizeof(cfg->passwd));
    if (pwd_len > MAX_PASSWD_LENGTH) {
        ESP_LOGW(TAG, "config invalid: wifi password len=%lu", (unsigned long)pwd_len);
        return false;
    }

    if (cfg->plant_config.lux > 2U) {
        ESP_LOGW(TAG, "config invalid: lux=%u", (unsigned)cfg->plant_config.lux);
        return false;
    }

    for (size_t i = 0; i < MOISTURE_THRESHOLD_COUNT; ++i) {
        if (cfg->plant_config.moi[i] > 100U) {
            ESP_LOGW(TAG, "config invalid: moi[%lu]=%u", (unsigned long)i, (unsigned)cfg->plant_config.moi[i]);
            return false;
        }
    }

    for (size_t i = 0; i < TEMP_THRESHOLD_COUNT; ++i) {
        if (cfg->plant_config.tem[i] == 0U) {
            ESP_LOGW(TAG, "config invalid: tem[%lu]=0", (unsigned long)i);
            return false;
        }
    }

    if (cfg->sleep_duration == 0U) {
        ESP_LOGW(TAG, "config invalid: sleep_duration=0");
        return false;
    }

    if (cfg->soil_adc_dry == 0U || cfg->soil_adc_wet == 0U) {
        ESP_LOGW(TAG, "config invalid: soil cal missing dry=%u wet=%u",
                 (unsigned)cfg->soil_adc_dry, (unsigned)cfg->soil_adc_wet);
        return false;
    }

    if (cfg->soil_adc_wet >= cfg->soil_adc_dry) {
        ESP_LOGW(TAG, "config invalid: soil cal order dry=%u wet=%u",
                 (unsigned)cfg->soil_adc_dry, (unsigned)cfg->soil_adc_wet);
        return false;
    }

    return true;
}

static esp_err_t save_meta(uint32_t next_seq, uint32_t stored)
{
    esp_err_t err = nvs_set_u32(s_nvs, NVS_KEY_META_NEXT, next_seq);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_set_u32(s_nvs, NVS_KEY_META_COUNT, stored);
    if (err != ESP_OK) {
        return err;
    }
    return nvs_commit(s_nvs);
}

static esp_err_t load_meta(uint32_t *next_seq, uint32_t *stored)
{
    esp_err_t err = nvs_get_u32(s_nvs, NVS_KEY_META_NEXT, next_seq);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        *next_seq = 0;
    } else if (err != ESP_OK) {
        return err;
    }

    err = nvs_get_u32(s_nvs, NVS_KEY_META_COUNT, stored);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        *stored = 0;
    } else if (err != ESP_OK) {
        return err;
    }

    return ESP_OK;
}

static void sample_key_from_index(uint32_t idx, char *out_key, size_t out_len)
{
    // idx is bounded by NVS_SENSOR_SAMPLES_N (<= 999) so 4 chars is enough
    snprintf(out_key, out_len, "s%03u", (unsigned)idx);
}

static esp_err_t erase_all_samples(uint32_t stored, uint32_t next_seq)
{
    if (stored == 0) {
        return ESP_OK;
    }

    uint32_t start_seq = (next_seq >= stored) ? (next_seq - stored) : 0;
    for (uint32_t i = 0; i < stored; ++i) {
        uint32_t seq = start_seq + i;
        uint32_t idx = seq % NVS_SENSOR_SAMPLES_N;
        char key[6];
        sample_key_from_index(idx, key, sizeof(key));
        (void)nvs_erase_key(s_nvs, key);
    }
    return nvs_commit(s_nvs);
}

/* =========================================================================
   SECTION: Public API
   ========================================================================= */
esp_err_t nvs_manager_init(void)
{
    return ensure_nvs();
}

esp_err_t nvs_manager_save_config(const config_t *cfg)
{
    if (cfg == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_RETURN_ON_ERROR(ensure_nvs(), TAG, "nvs not ready");

    esp_err_t err = nvs_set_blob(s_nvs, NVS_KEY_CONFIG, cfg, sizeof(*cfg));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "set config failed (%s)", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_u8(s_nvs, NVS_KEY_CONFIG_SET, 1);
    if (err != ESP_OK) {
        return err;
    }

    return nvs_commit(s_nvs);
}

esp_err_t nvs_manager_load_config(config_t *out_cfg, bool *has_config)
{
    if (out_cfg == NULL || has_config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_RETURN_ON_ERROR(ensure_nvs(), TAG, "nvs not ready");

    uint8_t flag = 0;
    esp_err_t err = nvs_get_u8(s_nvs, NVS_KEY_CONFIG_SET, &flag);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        *has_config = false;
        return ESP_OK;
    } else if (err != ESP_OK) {
        return err;
    }

    size_t required = sizeof(*out_cfg);
    err = nvs_get_blob(s_nvs, NVS_KEY_CONFIG, out_cfg, &required);
    if (err != ESP_OK) {
        return err;
    }

    if (!config_is_valid(out_cfg)) {
        ESP_LOGW(TAG, "config invalid; requiring reprovisioning");
        *has_config = false;
    } else {
        *has_config = true;
    }
    return ESP_OK;
}

esp_err_t nvs_manager_clear_config(void)
{
    ESP_RETURN_ON_ERROR(ensure_nvs(), TAG, "nvs not ready");

    (void)nvs_erase_key(s_nvs, NVS_KEY_CONFIG);
    (void)nvs_erase_key(s_nvs, NVS_KEY_CONFIG_SET);
    return nvs_commit(s_nvs);
}

esp_err_t nvs_manager_store_sample(sensor_sample_t *sample_in)
{
    if (sample_in == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_RETURN_ON_ERROR(ensure_nvs(), TAG, "nvs not ready");

    uint32_t next_seq = 0, stored = 0;
    ESP_RETURN_ON_ERROR(load_meta(&next_seq, &stored), TAG, "load meta failed");

    uint32_t seq = next_seq;
    uint32_t idx = seq % NVS_SENSOR_SAMPLES_N;
    char key[6];
    sample_key_from_index(idx, key, sizeof(key));

    sample_in->sample_seq = seq;
    esp_err_t err = nvs_set_blob(s_nvs, key, sample_in, sizeof(*sample_in));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "store sample failed (%s)", esp_err_to_name(err));
        return err;
    }

    next_seq += 1;
    if (stored < NVS_SENSOR_SAMPLES_N) {
        stored += 1;
    }

    ESP_RETURN_ON_ERROR(save_meta(next_seq, stored), TAG, "save meta failed");
    return ESP_OK;
}

esp_err_t nvs_manager_get_all_samples(sensor_sample_t *buffer, size_t max_items, size_t *out_count)
{
    if (buffer == NULL || out_count == NULL || max_items == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_RETURN_ON_ERROR(ensure_nvs(), TAG, "nvs not ready");

    uint32_t next_seq = 0, stored = 0;
    ESP_RETURN_ON_ERROR(load_meta(&next_seq, &stored), TAG, "load meta failed");

    size_t count = stored;
    if (count > max_items) {
        count = max_items;
    }

    if (count == 0) {
        *out_count = 0;
        return ESP_OK;
    }

    uint32_t start_seq = (next_seq >= count) ? (next_seq - count) : 0;
    for (size_t i = 0; i < count; ++i) {
        uint32_t seq = start_seq + i;
        uint32_t idx = seq % NVS_SENSOR_SAMPLES_N;
        char key[6];
        sample_key_from_index(idx, key, sizeof(key));

        size_t required = sizeof(sensor_sample_t);
        esp_err_t err = nvs_get_blob(s_nvs, key, &buffer[i], &required);
        if (err != ESP_OK) {
            return err;
        }

        buffer[i].sample_seq = seq; // enforce ordering even if blob missing seq
    }

    *out_count = count;
    return ESP_OK;
}

esp_err_t nvs_manager_clear_samples(void)
{
    ESP_RETURN_ON_ERROR(ensure_nvs(), TAG, "nvs not ready");

    uint32_t next_seq = 0, stored = 0;
    ESP_RETURN_ON_ERROR(load_meta(&next_seq, &stored), TAG, "load meta failed");

    ESP_RETURN_ON_ERROR(erase_all_samples(stored, next_seq), TAG, "erase samples failed");

    stored = 0;
    ESP_RETURN_ON_ERROR(save_meta(next_seq, stored), TAG, "save meta failed");
    return ESP_OK;
}

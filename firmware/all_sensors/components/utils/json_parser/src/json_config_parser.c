#include <string.h>
#include "esp_log.h"
#include "cJSON.h"
#include "json_config_parser.h"
#include "app_constants.h"

/* =========================================================================
   SECTION: Constants
   ========================================================================= */
#define JSON_CFG_MAX_LEN 256U
/* defaults are defined in app_constants.h */

/* =========================================================================
   SECTION: Static Data
   ========================================================================= */
static const char *TAG = "JSON_CFG";

/* =========================================================================
   SECTION: Public API
   ========================================================================= */
esp_err_t json_config_parse(const char *json_str_in, config_t *cfg_in_out)
{
    if (json_str_in == NULL || cfg_in_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t len = strnlen(json_str_in, JSON_CFG_MAX_LEN + 1U);
    if (len == 0U || len > JSON_CFG_MAX_LEN) {
        ESP_LOGW(TAG, "json len invalid (%u)", (unsigned)len);
        return ESP_ERR_INVALID_SIZE;
    }

    if (cfg_in_out->plant_config.moi[0] == 0U && cfg_in_out->plant_config.moi[1] == 0U) {
        cfg_in_out->plant_config.moi[0] = DEFAULT_MOI_MIN;
        cfg_in_out->plant_config.moi[1] = DEFAULT_MOI_MAX;
    }
    if (cfg_in_out->plant_config.tem[0] == 0U && cfg_in_out->plant_config.tem[1] == 0U) {
        cfg_in_out->plant_config.tem[0] = DEFAULT_TEM_MIN_DK;
        cfg_in_out->plant_config.tem[1] = DEFAULT_TEM_MAX_DK;
    }
    if (cfg_in_out->mes == 0U) {
        cfg_in_out->mes = DEFAULT_MES_S;
    }
    if (cfg_in_out->sen == 0U) {
        cfg_in_out->sen = DEFAULT_SEN_S;
    }
    if (cfg_in_out->wat == 0U) {
        cfg_in_out->wat = DEFAULT_WAT_S;
    }
    if (cfg_in_out->wai == 0U) {
        cfg_in_out->wai = DEFAULT_WAI_S;
    }

    cJSON *root = cJSON_ParseWithLength(json_str_in, len);
    if (root == NULL) {
        ESP_LOGW(TAG, "json parse failed, using defaults");
        return ESP_OK;
    }

    const cJSON *moi = cJSON_GetObjectItem(root, "moi");
    const cJSON *tem = cJSON_GetObjectItem(root, "tem");
    const cJSON *mes = cJSON_GetObjectItem(root, "mes");
    const cJSON *sen = cJSON_GetObjectItem(root, "sen");
    const cJSON *wat = cJSON_GetObjectItem(root, "wat");
    const cJSON *wai = cJSON_GetObjectItem(root, "wai");

    if (cJSON_IsArray(moi) && cJSON_GetArraySize(moi) == 2) {
        const cJSON *moi_min = cJSON_GetArrayItem(moi, 0);
        const cJSON *moi_max = cJSON_GetArrayItem(moi, 1);
        if (cJSON_IsNumber(moi_min) && cJSON_IsNumber(moi_max)) {
            int moi_min_val = (int)cJSON_GetNumberValue(moi_min);
            int moi_max_val = (int)cJSON_GetNumberValue(moi_max);
            if (moi_min_val >= 0 && moi_min_val <= 100 && moi_max_val >= 0 && moi_max_val <= 100) {
                cfg_in_out->plant_config.moi[0] = (uint8_t)moi_min_val;
                cfg_in_out->plant_config.moi[1] = (uint8_t)moi_max_val;
                ESP_LOGI(TAG, "cfg moi=[%d,%d]", moi_min_val, moi_max_val);
            } else {
                ESP_LOGI(TAG, "cfg moi=<invalid>");
            }
        } else {
            ESP_LOGI(TAG, "cfg moi=<invalid>");
        }
    } else if (moi != NULL) {
        ESP_LOGI(TAG, "cfg moi=<invalid>");
    }

    if (cJSON_IsArray(tem) && cJSON_GetArraySize(tem) == 2) {
        const cJSON *tem_min = cJSON_GetArrayItem(tem, 0);
        const cJSON *tem_max = cJSON_GetArrayItem(tem, 1);
        if (cJSON_IsNumber(tem_min) && cJSON_IsNumber(tem_max)) {
            double tem_min_val = (double)cJSON_GetNumberValue(tem_min);
            double tem_max_val = (double)cJSON_GetNumberValue(tem_max);
            double tem_min_k = (tem_min_val < 200.0) ? (tem_min_val + 273.15) : tem_min_val;
            double tem_max_k = (tem_max_val < 200.0) ? (tem_max_val + 273.15) : tem_max_val;
            uint32_t tem_min_dk = (tem_min_k > 0.0) ? (uint32_t)(tem_min_k * 10.0 + 0.5) : 0U;
            uint32_t tem_max_dk = (tem_max_k > 0.0) ? (uint32_t)(tem_max_k * 10.0 + 0.5) : 0U;
            if (tem_min_dk <= 65535U && tem_max_dk <= 65535U) {
                cfg_in_out->plant_config.tem[0] = (uint16_t)tem_min_dk;
                cfg_in_out->plant_config.tem[1] = (uint16_t)tem_max_dk;
                ESP_LOGI(TAG, "cfg tem=[%.1f,%.1f]", tem_min_val, tem_max_val);
            } else {
                ESP_LOGI(TAG, "cfg tem=<invalid>");
            }
        } else {
            ESP_LOGI(TAG, "cfg tem=<invalid>");
        }
    } else if (tem != NULL) {
        ESP_LOGI(TAG, "cfg tem=<invalid>");
    }

    if (cJSON_IsNumber(mes)) {
        int mes_val = (int)cJSON_GetNumberValue(mes);
        if (mes_val > 0 && mes_val <= 65535) {
            cfg_in_out->mes = (uint16_t)mes_val;
            ESP_LOGI(TAG, "cfg mes=%d", mes_val);
        } else {
            ESP_LOGI(TAG, "cfg mes=<invalid>");
        }
    } else if (mes != NULL) {
        ESP_LOGI(TAG, "cfg mes=<invalid>");
    }

    if (cJSON_IsNumber(sen)) {
        int sen_val = (int)cJSON_GetNumberValue(sen);
        if (sen_val > 0 && sen_val <= 65535) {
            cfg_in_out->sen = (uint16_t)sen_val;
            ESP_LOGI(TAG, "cfg sen=%d", sen_val);
        } else {
            ESP_LOGI(TAG, "cfg sen=<invalid>");
        }
    } else if (sen != NULL) {
        ESP_LOGI(TAG, "cfg sen=<invalid>");
    }


    if (cJSON_IsNumber(wat)) {
        int wat_val = (int)cJSON_GetNumberValue(wat);
        if (wat_val >= 0 && wat_val <= 65535) {
            cfg_in_out->wat = (uint16_t)wat_val;
            ESP_LOGI(TAG, "cfg wat=%d", wat_val);
        } else {
            ESP_LOGI(TAG, "cfg wat=<invalid>");
        }
    } else if (wat != NULL) {
        ESP_LOGI(TAG, "cfg wat=<invalid>");
    }

    if (cJSON_IsNumber(wai)) {
        int wai_val = (int)cJSON_GetNumberValue(wai);
        if (wai_val >= 0 && wai_val <= 65535) {
            cfg_in_out->wai = (uint16_t)wai_val;
            ESP_LOGI(TAG, "cfg wai=%d", wai_val);
        } else {
            ESP_LOGI(TAG, "cfg wai=<invalid>");
        }
    } else if (wai != NULL) {
        ESP_LOGI(TAG, "cfg wai=<invalid>");
    }

    cJSON_Delete(root);
    return ESP_OK;
}

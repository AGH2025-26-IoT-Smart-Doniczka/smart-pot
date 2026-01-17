#include "esp_log.h"
#include "esp_err.h"
#include "app_context.h"
#include "nvs_manager.h"
#include "fsm_manager.h"

static const char *TAG = "APP";

void app_main(void)
{
	ESP_LOGI(TAG, "booting");

	ESP_ERROR_CHECK(app_context_init());
	ESP_ERROR_CHECK(nvs_manager_init());
	ESP_ERROR_CHECK(fsm_manager_init(NULL));
}

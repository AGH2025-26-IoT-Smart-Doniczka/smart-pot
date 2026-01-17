#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_master.h"
#include "ssd1306_types.h"

#ifdef __cplusplus
#error "This project uses C only."
#endif

/* =========================================================================
   SECTION: Forward Declarations
   ========================================================================= */
typedef struct ssd1306 ssd1306_t;
typedef ssd1306_t *ssd1306_handle_t;

/* =========================================================================
   SECTION: API
   ========================================================================= */
esp_err_t ssd1306_create(i2c_master_bus_handle_t bus, ssd1306_handle_t *out_handle);
void ssd1306_destroy(ssd1306_handle_t handle);
esp_err_t ssd1306_power_off(ssd1306_handle_t handle);

esp_err_t ssd1306_clear(ssd1306_handle_t handle);
esp_err_t ssd1306_fill_rect(ssd1306_handle_t handle, uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool color);
esp_err_t ssd1306_draw_text(ssd1306_handle_t handle, const char *text, uint8_t x, uint8_t page);
esp_err_t ssd1306_draw_bitmap(ssd1306_handle_t handle, const ssd1306_image_t *image);
esp_err_t ssd1306_flush(ssd1306_handle_t handle);
esp_err_t ssd1306_draw_button_label_left(ssd1306_handle_t handle, const char *label);
esp_err_t ssd1306_draw_button_label_right(ssd1306_handle_t handle, const char *label);

#pragma once

#include <stdint.h>

#ifdef __cplusplus
#error "This project uses C only."
#endif

/* =========================================================================
   SECTION: Constants
   ========================================================================= */
#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64

/* =========================================================================
   SECTION: Types
   ========================================================================= */
typedef struct {
    uint8_t x;
    uint8_t y;
    uint8_t width;
    uint8_t height;
    const uint8_t *data;
} ssd1306_image_t;

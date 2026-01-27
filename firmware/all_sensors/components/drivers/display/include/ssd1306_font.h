#pragma once

#include <stdint.h>

#ifdef __cplusplus
#error "This project uses C only."
#endif

/* =========================================================================
   SECTION: Constants
   ========================================================================= */
#define SSD1306_FONT_WIDTH 8
#define SSD1306_FONT_HEIGHT 8

/* =========================================================================
   SECTION: API
   ========================================================================= */
const uint8_t *ssd1306_font_get_glyph(uint32_t codepoint);

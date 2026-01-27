#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_check.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "ssd1306.h"
#include "ssd1306_font.h"

/* =========================================================================
   SECTION: Constants
   ========================================================================= */
#define SSD1306_I2C_ADDRESS           0x3C
#define SSD1306_I2C_CLOCK_HZ          100000
#define SSD1306_CONTROL_BYTE_COMMAND  0x00
#define SSD1306_CONTROL_BYTE_DATA     0x40
#define SSD1306_BUFFER_SIZE          ((SSD1306_WIDTH * SSD1306_HEIGHT / 8) + 1)
#define SSD1306_MUTEX_TIMEOUT        pdMS_TO_TICKS(500)
#define SSD1306_I2C_TIMEOUT_MS       100

/* =========================================================================
   SECTION: Types
   ========================================================================= */
struct ssd1306 {
    i2c_master_bus_handle_t bus;
    i2c_master_dev_handle_t dev;
    SemaphoreHandle_t lock;
    bool need_reinit;
    uint8_t buffer[SSD1306_BUFFER_SIZE];
};

/* =========================================================================
   SECTION: Static Data
   ========================================================================= */
static const char *TAG = "ssd1306";
static ssd1306_t s_ctx = {0};

static const uint8_t s_init_sequence[] = {
    SSD1306_CONTROL_BYTE_COMMAND,
    0xAE,       // Display OFF
    0x20, 0x00, // Horizontal addressing mode
    0xA1,       // Segment remap
    0xC8,       // COM scan direction
    0xDA, 0x12, // COM pins hardware config
    0xD5, 0x80, // Display clock divide
    0xA8, 0x3F, // Multiplex ratio (64)
    0xD3, 0x00, // Display offset
    0x40,       // Start line
    0x8D, 0x14, // Charge pump enable
    0x81, 0x01, // Contrast
    0xD9, 0xF1, // Pre-charge period
    0xDB, 0x40, // VCOMH deselect level
    0xA4,       // Entire display ON follows RAM
    0xAF        // Display ON
};

/* =========================================================================
   SECTION: Helpers
   ========================================================================= */
static bool ssd1306_lock(ssd1306_t *ctx) {
    if (ctx == NULL || ctx->lock == NULL) {
        return false;
    }
    return xSemaphoreTake(ctx->lock, SSD1306_MUTEX_TIMEOUT) == pdTRUE;
}

static void ssd1306_unlock(ssd1306_t *ctx) {
    if (ctx && ctx->lock) {
        xSemaphoreGive(ctx->lock);
    }
}

static esp_err_t ssd1306_send_commands(ssd1306_t *ctx, const uint8_t *cmds, size_t len) {
    if (len == 0) {
        return ESP_OK;
    }
    return i2c_master_transmit(ctx->dev, cmds, len, SSD1306_I2C_TIMEOUT_MS);
}

static esp_err_t ssd1306_reset_cursor(ssd1306_t *ctx) {
    const uint8_t cmds[] = {
        SSD1306_CONTROL_BYTE_COMMAND,
        0x21, 0x00, 0x7F, // Column address
        0x22, 0x00, 0x07  // Page address
    };
    return ssd1306_send_commands(ctx, cmds, sizeof(cmds));
}

static esp_err_t ssd1306_try_recover(ssd1306_t *ctx) {
    esp_err_t err = ssd1306_send_commands(ctx, s_init_sequence, sizeof(s_init_sequence));
    if (err == ESP_OK) {
        ctx->need_reinit = false;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    return err;
}

static inline void ssd1306_draw_pixel_locked(ssd1306_t *ctx, uint8_t x, uint8_t y, bool color) {
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
        return;
    }

    uint8_t page = y / 8U;
    uint8_t bit_index = y % 8U;
    size_t buffer_index = (page * SSD1306_WIDTH) + x + 1U;

    if (color) {
        ctx->buffer[buffer_index] |= (1U << bit_index);
    } else {
        ctx->buffer[buffer_index] &= (uint8_t)~(1U << bit_index);
    }
}

static int ssd1306_decode_utf8(const char *text, uint32_t *codepoint_out) {
    const unsigned char *u = (const unsigned char *)text;
    if ((u[0] & 0x80U) == 0) {
        *codepoint_out = u[0];
        return 1;
    }
    if ((u[0] & 0xE0U) == 0xC0U) {
        *codepoint_out = ((uint32_t)(u[0] & 0x1FU) << 6) | (u[1] & 0x3FU);
        return 2;
    }
    if ((u[0] & 0xF0U) == 0xE0U) {
        *codepoint_out = ((uint32_t)(u[0] & 0x0FU) << 12) | ((uint32_t)(u[1] & 0x3FU) << 6) | (u[2] & 0x3FU);
        return 3;
    }
    *codepoint_out = '?';
    return 1;
}

static esp_err_t ssd1306_init_device(ssd1306_t *ctx, i2c_master_bus_handle_t bus) {
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SSD1306_I2C_ADDRESS,
        .scl_speed_hz = SSD1306_I2C_CLOCK_HZ,
    };

    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(bus, &dev_cfg, &ctx->dev), TAG, "add device failed");
    vTaskDelay(pdMS_TO_TICKS(20)); // allow display power-up

    esp_err_t err = ESP_FAIL;
    for (int attempt = 0; attempt < 3; ++attempt) {
        err = ssd1306_send_commands(ctx, s_init_sequence, sizeof(s_init_sequence));
        if (err == ESP_OK) {
            break;
        }
        ESP_LOGW(TAG, "init sequence failed (attempt %d): %s", attempt + 1, esp_err_to_name(err));
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    if (err != ESP_OK) {
        ctx->need_reinit = true;
        return err;
    }

    ctx->need_reinit = false;
    return ESP_OK;
}

/* =========================================================================
   SECTION: API
   ========================================================================= */
esp_err_t ssd1306_create(i2c_master_bus_handle_t bus, ssd1306_handle_t *out_handle) {
    ESP_RETURN_ON_FALSE(bus != NULL, ESP_ERR_INVALID_ARG, TAG, "bus is null");
    ESP_RETURN_ON_FALSE(out_handle != NULL, ESP_ERR_INVALID_ARG, TAG, "out_handle is null");

    if (s_ctx.dev != NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    s_ctx.lock = xSemaphoreCreateMutex();
    if (s_ctx.lock == NULL) {
        return ESP_ERR_NO_MEM;
    }

    s_ctx.bus = bus;
    s_ctx.buffer[0] = SSD1306_CONTROL_BYTE_DATA;

    esp_err_t err = ssd1306_init_device(&s_ctx, bus);
    if (err != ESP_OK) {
        ssd1306_destroy(&s_ctx);
        return err;
    }

    err = ssd1306_clear(&s_ctx);
    if (err == ESP_OK) {
        err = ssd1306_flush(&s_ctx);
    }

    if (err != ESP_OK) {
        ssd1306_destroy(&s_ctx);
        return err;
    }

    *out_handle = &s_ctx;
    return ESP_OK;
}

void ssd1306_destroy(ssd1306_handle_t handle) {
    ssd1306_t *ctx = handle ? handle : &s_ctx;

    if (ctx->dev != NULL) {
        i2c_master_bus_rm_device(ctx->dev);
        ctx->dev = NULL;
    }
    if (ctx->lock != NULL) {
        vSemaphoreDelete(ctx->lock);
        ctx->lock = NULL;
    }
    memset(ctx, 0, sizeof(*ctx));
}

esp_err_t ssd1306_power_off(ssd1306_handle_t handle) {
    ssd1306_t *ctx = handle;
    ESP_RETURN_ON_FALSE(ctx != NULL, ESP_ERR_INVALID_ARG, TAG, "handle is null");
    ESP_RETURN_ON_FALSE(ctx->dev != NULL, ESP_ERR_INVALID_STATE, TAG, "device not initialized");
    if (!ssd1306_lock(ctx)) {
        return ESP_ERR_TIMEOUT;
    }

    const uint8_t cmds[] = {
        SSD1306_CONTROL_BYTE_COMMAND,
        0xAE
    };
    esp_err_t err = ssd1306_send_commands(ctx, cmds, sizeof(cmds));
    ssd1306_unlock(ctx);
    return err;
}

esp_err_t ssd1306_clear(ssd1306_handle_t handle) {
    ssd1306_t *ctx = handle;
    ESP_RETURN_ON_FALSE(ctx != NULL, ESP_ERR_INVALID_ARG, TAG, "handle is null");
    if (!ssd1306_lock(ctx)) {
        return ESP_ERR_TIMEOUT;
    }
    memset(ctx->buffer, 0, sizeof(ctx->buffer));
    ctx->buffer[0] = SSD1306_CONTROL_BYTE_DATA;
    ssd1306_unlock(ctx);
    return ESP_OK;
}

esp_err_t ssd1306_fill_rect(ssd1306_handle_t handle, uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool color) {
    ssd1306_t *ctx = handle;
    ESP_RETURN_ON_FALSE(ctx != NULL, ESP_ERR_INVALID_ARG, TAG, "handle is null");
    if (!ssd1306_lock(ctx)) {
        return ESP_ERR_TIMEOUT;
    }

    for (uint8_t dx = 0; dx < w; ++dx) {
        for (uint8_t dy = 0; dy < h; ++dy) {
            ssd1306_draw_pixel_locked(ctx, (uint8_t)(x + dx), (uint8_t)(y + dy), color);
        }
    }

    ssd1306_unlock(ctx);
    return ESP_OK;
}

esp_err_t ssd1306_draw_bitmap(ssd1306_handle_t handle, const ssd1306_image_t *image) {
    ssd1306_t *ctx = handle;
    ESP_RETURN_ON_FALSE(ctx != NULL, ESP_ERR_INVALID_ARG, TAG, "handle is null");
    ESP_RETURN_ON_FALSE(image != NULL, ESP_ERR_INVALID_ARG, TAG, "image is null");
    if (!ssd1306_lock(ctx)) {
        return ESP_ERR_TIMEOUT;
    }

    const uint8_t *data = image->data;
    for (uint8_t y = 0; y < image->height; ++y) {
        if ((uint16_t)image->y + y >= SSD1306_HEIGHT) {
            break;
        }
        for (uint8_t x = 0; x < image->width; ++x) {
            if ((uint16_t)image->x + x >= SSD1306_WIDTH) {
                break;
            }
            size_t bit_index = (size_t)y * image->width + x;
            size_t byte_index = bit_index / 8U;
            uint8_t bit_pos = 7U - (bit_index % 8U);
            bool color = (data[byte_index] & (uint8_t)(1U << bit_pos)) != 0U;
            ssd1306_draw_pixel_locked(ctx, (uint8_t)(image->x + x), (uint8_t)(image->y + y), color);
        }
    }

    ssd1306_unlock(ctx);
    return ESP_OK;
}

esp_err_t ssd1306_draw_text(ssd1306_handle_t handle, const char *text, uint8_t x, uint8_t page) {
    ssd1306_t *ctx = handle;
    ESP_RETURN_ON_FALSE(ctx != NULL, ESP_ERR_INVALID_ARG, TAG, "handle is null");
    ESP_RETURN_ON_FALSE(text != NULL, ESP_ERR_INVALID_ARG, TAG, "text is null");
    if (page >= (SSD1306_HEIGHT / 8U) || x > (SSD1306_WIDTH - SSD1306_FONT_WIDTH)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!ssd1306_lock(ctx)) {
        return ESP_ERR_TIMEOUT;
    }

    uint8_t cursor_x = x;
    uint8_t cursor_page = page;
    const uint8_t base_x = x;

    while (*text != '\0' && cursor_page < (SSD1306_HEIGHT / 8U)) {
        uint32_t codepoint = 0;
        int consumed = ssd1306_decode_utf8(text, &codepoint);

        if (codepoint == '\n') {
            cursor_x = base_x;
            ++cursor_page;
            text += consumed;
            continue;
        }

        if (cursor_x > SSD1306_WIDTH - SSD1306_FONT_WIDTH) {
            cursor_x = base_x;
            ++cursor_page;
            if (cursor_page >= (SSD1306_HEIGHT / 8U)) {
                break;
            }
        }

        const uint8_t *glyph = ssd1306_font_get_glyph(codepoint);
        size_t base_index = ((size_t)cursor_page * SSD1306_WIDTH) + cursor_x + 1U;
        for (uint8_t col = 0; col < SSD1306_FONT_WIDTH; ++col) {
            if ((uint16_t)cursor_x + col >= SSD1306_WIDTH) {
                break;
            }
            ctx->buffer[base_index + col] = glyph[col];
        }

        cursor_x = (uint8_t)(cursor_x + SSD1306_FONT_WIDTH);
        text += consumed;
    }

    ssd1306_unlock(ctx);
    return ESP_OK;
}


esp_err_t ssd1306_draw_button_label_left(ssd1306_handle_t handle, const char *label) {
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "handle is null");
    ESP_RETURN_ON_FALSE(label != NULL, ESP_ERR_INVALID_ARG, TAG, "label is null");

    char buf[8] = {0}; 
    size_t len = strnlen(label, sizeof(buf) - 1U);
    memcpy(buf, label, len);

    uint8_t page = (SSD1306_HEIGHT / 8U) - 1U;


    (void)ssd1306_draw_text(handle, "        ", 0, page);

    uint8_t x_bracket = 0;             
    uint8_t x_label = SSD1306_FONT_WIDTH; 

    ESP_LOGD(TAG, "btn-left label='%s' len=%u x_label=%u", buf, (unsigned)len, (unsigned)x_label);

    (void)ssd1306_draw_text(handle, "<", x_bracket, page);
    (void)ssd1306_draw_text(handle, buf, x_label, page);

    return ESP_OK;
}

esp_err_t ssd1306_draw_button_label_right(ssd1306_handle_t handle, const char *label) {
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "handle is null");
    ESP_RETURN_ON_FALSE(label != NULL, ESP_ERR_INVALID_ARG, TAG, "label is null");

    char buf[8] = {0}; // max 7 chars + null
    size_t len = strnlen(label, sizeof(buf) - 1U);
    memcpy(buf, label, len);

    uint8_t page = (SSD1306_HEIGHT / 8U) - 1U;

    // Clear slots 8-15 (8 chars) on bottom line
    (void)ssd1306_draw_text(handle, "        ", (uint8_t)(8 * SSD1306_FONT_WIDTH), page);

    uint8_t start_slot = (uint8_t)(16 - (len + 1));
    if (start_slot < 8U) {
        start_slot = 8U;
    }
    uint8_t x_label = (uint8_t)(start_slot * SSD1306_FONT_WIDTH);
    uint8_t x_bracket = (uint8_t)(15U * SSD1306_FONT_WIDTH); 

    ESP_LOGD(TAG, "btn-right label='%s' len=%u start_slot=%u x_label=%u x_right=%u", buf, (unsigned)len, (unsigned)start_slot, (unsigned)x_label, (unsigned)x_bracket);

    (void)ssd1306_draw_text(handle, buf, x_label, page);
    (void)ssd1306_draw_text(handle, ">", x_bracket, page);

    return ESP_OK;
}

esp_err_t ssd1306_flush(ssd1306_handle_t handle) {
    ssd1306_t *ctx = handle;
    ESP_RETURN_ON_FALSE(ctx != NULL, ESP_ERR_INVALID_ARG, TAG, "handle is null");
    if (!ssd1306_lock(ctx)) {
        return ESP_ERR_TIMEOUT;
    }

    if (ctx->need_reinit) {
        if (ssd1306_try_recover(ctx) != ESP_OK) {
            ssd1306_unlock(ctx);
            return ESP_FAIL;
        }
    }

    esp_err_t err = ssd1306_reset_cursor(ctx);
    if (err != ESP_OK) {
        ctx->need_reinit = true;
        ssd1306_unlock(ctx);
        return err;
    }

    err = i2c_master_transmit(ctx->dev, ctx->buffer, sizeof(ctx->buffer), SSD1306_I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        ctx->need_reinit = true;
    }

    ssd1306_unlock(ctx);
    return err;
}

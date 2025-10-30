#include "display.h"
#include <string.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <board.h>
#define COLOR_EMPTY  0,0,0
#define COLOR_P1     255,0,0
#define COLOR_P2     0,0,255

bool frame_change = true;

static Pixel4b frameBuffer[MATRIX_H][MATRIX_W];

// Fester Puffer für I2C (Stack, keine VLAs)
static uint8_t i2c_buffer[1 + MATRIX_W * MATRIX_H * 2];

void display_mark_changed() {
    frame_change = true;
}

// --- I2C write ---
static void i2c_write(uint8_t reg, const uint8_t *data, size_t len) {
    uint8_t buf[1 + 128]; // feste Größe für kleine Matrix, len <= 128
    buf[0] = reg;
    if (data && len > 0) {
        memcpy(&buf[1], data, len);
    }
    i2c_master_write_to_device(DISPLAY_I2C_PORT, DISPLAY_ADDR, buf, len + 1, 50 / portTICK_PERIOD_MS);
}

// --- Init I2C + Display ---
void display_init() {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 8,
        .scl_io_num = 9,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = DISPLAY_I2C_FREQ
    };
    i2c_param_config(DISPLAY_I2C_PORT, &conf);
    i2c_driver_install(DISPLAY_I2C_PORT, I2C_MODE_MASTER, 0, 0, 0);

    display_clear();
    display_update();
}

// --- Display Clear ---
void display_clear() {
    memset(frameBuffer, 0, sizeof(frameBuffer));
    frame_change = true;
}

// --- Set Pixel ---
void display_set_pixel(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b) {
    if (x >= MATRIX_W || y >= MATRIX_H) return;
    frameBuffer[y][x].r = (r >> 4) & 0x0F; // 0-255 → 0-15
    frameBuffer[y][x].g = (g >> 4) & 0x0F;
    frameBuffer[y][x].b = (b >> 4) & 0x0F;
    frame_change = true;
}

// --- Update Display ---
void display_update() {
    int idx = 0;
    for (int y=0; y<MATRIX_H; y++) {
        for (int x=0; x<MATRIX_W; x++) {
            Pixel4b px = frameBuffer[y][x];
            i2c_buffer[idx++] = (px.r << 4) | px.g; // R+G in einem Byte
            i2c_buffer[idx++] = (px.b << 4);        // B in oberem Nibble
        }
    }
    i2c_write(0x10, i2c_buffer, idx);
    i2c_write(0x20, NULL, 0); // Page-Flip
}


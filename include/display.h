#ifndef SLP_DISPLAY_H
#define SLP_DISPLAY_H
#pragma once
#include "driver/i2c.h"
#include <stdbool.h>

#define MATRIX_W 8
#define MATRIX_H 8

#define DISPLAY_I2C_PORT I2C_NUM_0
#define DISPLAY_I2C_FREQ 400000
#define DISPLAY_ADDR 0x74  // anpassbar


extern bool frame_change;

typedef struct {
    uint8_t r : 4; // 0-15
    uint8_t g : 4; // 0-15
    uint8_t b : 4; // 0-15
} Pixel4b;

// API
void display_mark_changed();
void display_init();
void display_clear();
void display_set_pixel(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b);
void display_update();
void display_task();

#endif //SLP_DISPLAY_H

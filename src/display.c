#include "display.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>
#include "board.h"
#include <stdio.h>

#define TAG "DISPLAY_UART"

#define UART_PORT_NUM UART_NUM_1
#define UART_BAUD     9600
#define TXD_PIN       21
#define RXD_PIN       22   // nur senden nötig

#define MATRIX_W 8
#define MATRIX_H 8

bool frame_change = true;
static Pixel4b frameBuffer[MATRIX_H][MATRIX_W];

// --- Init UART ---
void display_init() {
    const uart_config_t uart_config = {
        .baud_rate = UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    uart_param_config(UART_PORT_NUM, &uart_config);
    uart_set_pin(UART_PORT_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_PORT_NUM, 2048, 0, 0, NULL, 0);

    ESP_LOGI(TAG, "UART Display init OK (TX=%d, Baud=%d)", TXD_PIN, UART_BAUD);
}

// --- Markiere Frame als geändert ---
void display_mark_changed() { frame_change = true; }

// --- Board ins FrameBuffer kopieren ---
void display_board_to_buffer() {
    for (uint8_t y=0; y<MATRIX_H; y++) {
        for (uint8_t x=0; x<MATRIX_W; x++) {
            uint8_t val = board_get(x, y);
            if (val == 1) frameBuffer[y][x] = (Pixel4b){15,0,0};
            else if (val == 2) frameBuffer[y][x] = (Pixel4b){0,0,15};
            else frameBuffer[y][x] = (Pixel4b){0,0,0};
        }
    }
}

// --- Update: komplettes Board senden ---
void display_update_board() {
    display_board_to_buffer();

    uint8_t buf[MATRIX_W * MATRIX_H * 3];
    int idx = 0;

    for (uint8_t y=0; y<MATRIX_H; y++) {
        for (uint8_t x=0; x<MATRIX_W; x++) {
            Pixel4b px = frameBuffer[y][x];
            buf[idx++] = px.r;
            buf[idx++] = px.g;
            buf[idx++] = px.b;
        }
    }

    ESP_LOGI(TAG, "Display update board OK");
    printf("%d ",sizeof(buf));
    uart_write_bytes(UART_PORT_NUM, (const char*)buf, sizeof(buf));
    frame_change = false;

}

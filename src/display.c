#include "display.h"
#include "driver/uart.h"
#include "board.h"
#include <driver/i2c.h>
#include "esp_log.h"
#include <esp_timer.h>
#include <freertos/task.h>
#include <portmacro.h>
#define TAG "Display"
#define UART_PORT UART_NUM_1
#define TX_PIN 21   // wähle passende Pins
#define RX_PIN 22


void display_init() {
    const uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_driver_install(UART_PORT, 1024, 0, 0, NULL, 0);
    uart_param_config(UART_PORT, &uart_config);
    uart_set_pin(UART_PORT, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}
void sendChangesToDisplay() {
    ESP_LOGI(TAG, "Size of Board: %zu", sizeof(board));
    ESP_LOGI(TAG, "Sending changes to display");
    int bytes = uart_write_bytes(UART_PORT, (const char*)board, BOARD_BYTES);
    ESP_LOGI(TAG, "Sent %d bytes over UART", bytes);
}

void display_task() {
    const TickType_t delay = pdMS_TO_TICKS(1000); // Polling alle 20ms
    display_init();

    while (true) {

        sendChangesToDisplay();

        vTaskDelay(delay);

    }
}


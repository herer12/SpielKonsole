#include "render.h"
#include "board.h"
#include "display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Spielerfarben
#define COLOR_EMPTY  0, 0, 0
#define COLOR_P1   255, 0, 0
#define COLOR_P2     0, 0, 255

void renderer_init() {
    display_init();
}

void renderer_task() {
    TickType_t period = pdMS_TO_TICKS(30);
    while (true) {
        display_update_board();
        vTaskDelay(period);
    }
}
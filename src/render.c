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
    display_clear();
    display_update();
}

void display_task() {
    TickType_t period = pdMS_TO_TICKS(30);
    while (true) {
        if (frame_change) {
            display_clear();
            // Lies Board und male es
            for (int y = 0; y < 8; y++) {
                for (int x = 0; x < 8; x++) {
                    uint8_t val = board_get(x, y);
                    if (val == 1) display_set_pixel(x, y, COLOR_P1);
                    else if (val == 2) display_set_pixel(x, y, COLOR_P2);
                    // leere Felder schwarz
                }
            }
            display_update();
            frame_change = false;
        }
        vTaskDelay(period);
    }
}
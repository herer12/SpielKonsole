#include "display.h"
#include "board.h"
#include <driver/i2c.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/task.h>
#include <portmacro.h>
#define TAG "Display"

void display_init() {

}
void sendChangesToDisplay() {
    ESP_LOGI(TAG, "Size of Board" ,sizeof(get_Board()));

    ESP_LOGI(TAG, "Sending changes to display", );
}

void display_task() {
    const TickType_t delay = pdMS_TO_TICKS(20); // Polling alle 20ms

    while (true) {
        sendChangesToDisplay();
        vTaskDelay(delay);
    }
}

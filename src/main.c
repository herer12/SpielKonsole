#include <string.h>
#include "game_conect4.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define TAG "Main menü"

// ===== Hauptmenü =====
#define GAME_COUNT 1
typedef enum {
    GAME_CONNECT4,
} GameType;

GameType current_game = GAME_CONNECT4;


// ===== GPIO Pins =====
#define LEFT_BTN   2
#define RIGHT_BTN  3
#define DOWN_BTN   4
#define UP_BTN     5
#define A_BTN      6
#define B_BTN      7
#define HOME_BTN   8
static const uint8_t pinSteuerkreuz[] = {LEFT_BTN, RIGHT_BTN, DOWN_BTN, UP_BTN};
static const uint8_t numberOfPin = sizeof(pinSteuerkreuz) / sizeof(pinSteuerkreuz[0]);
#define BUTTON_ACTIVE_LEVEL 0  // 0 für active-low, 1 für active-high

// ===== I2C Config =====

// ===== Entprellung =====
#define DEBOUNCE_MS 200
static uint64_t last_press_times[8] = {0};

// ===== Prototypes =====
void buttons_init(void);
bool button_pressed(int btn);
int getButtonPressed();
void button_task(void *pvParameter);
void handle_home_button();
void button_handler_different_games(uint8_t pin);


// ============================================================================
// MAIN LOOP
// ============================================================================

void app_main(void) {
    ESP_LOGI(TAG, "Startet Main");

    buttons_init();

    logic_connect4_init();

    // Button-Task starten
    //17% Stack mehr als du brauchst
    xTaskCreate(button_task, "button_task", 2000, NULL, 7, NULL);

    /*
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
        //DisplayLogic
        // switch (current_game) {
        //     case GAME_CONNECT4:break;
        // }
    }
    */


}


// ============================================================================
// BUTTONS
// ============================================================================

void buttons_init(void) {
    // GPIOs konfigurieren
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE, // Keine Interrupts nötig
    };

    for (int i = 0; i < numberOfPin; i++) {
        io_conf.pin_bit_mask = 1ULL << pinSteuerkreuz[i];
        gpio_config(&io_conf);
    }


}

void button_task(void *pvParameter) {
    const TickType_t delay = pdMS_TO_TICKS(20); // Polling alle 20ms

    while (1) {
        uint64_t now = esp_timer_get_time() / 1000; // in ms
        for (int i = 0; i < numberOfPin; i++) {
            int pin = pinSteuerkreuz[i];
            int level = gpio_get_level(pin);

            if (level == BUTTON_ACTIVE_LEVEL && (now - last_press_times[i] > DEBOUNCE_MS)) {
                last_press_times[i] = now;
                ESP_LOGI(TAG, "Button %d pressed", pin);
                button_handler_different_games(pin);
            }
        }

        vTaskDelay(delay);

    }
}
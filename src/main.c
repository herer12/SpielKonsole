#include <stdio.h>
#include <string.h>
#include "display.h"
#include "game_conect4.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "render.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <portmacro.h>
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
#define HOME_BTN   10
static const uint8_t pinSteuerkreuz[] = {LEFT_BTN, RIGHT_BTN, DOWN_BTN, UP_BTN};
#define NUMBER_OF_PIN (sizeof(pinSteuerkreuz) / sizeof(pinSteuerkreuz[0]))
#define BUTTON_ACTIVE_LEVEL 0  // 0 für active-low, 1 für active-high

// ===== I2C Config =====

// ===== Entprellung =====
#define DEBOUNCE_MS 200
static uint32_t last_press_times[8] = {0};

// ===== Prototypes =====
void buttons_init();
bool button_pressed(int btn);
int getButtonPressed();
void button_task();
void handle_home_button();
void button_handler_different_games(uint8_t pin);
void smiley_animation();

void i2c_scan() {
    printf("I2C scan:\n");
    for (uint8_t addr = 1; addr < 127; addr++) {
        if (i2c_master_write_to_device(I2C_NUM_0, addr, NULL, 0, 10 / portTICK_PERIOD_MS) == ESP_OK) {
            printf("Found device at 0x%02X\n", addr);
        }
    }
}


// ============================================================================
// MAIN LOOP
// ============================================================================

void app_main(void) {
    ESP_LOGI(TAG, "Startet Main");

    buttons_init();

    logic_connect4_init();

    renderer_init();

    //i2c_scan();

    //smiley_animation();

    //Display Task starten
    xTaskCreate(renderer_task, "display_task", 2048, NULL, 5, NULL);



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

void buttons_init() {
    // GPIOs konfigurieren
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE, // Keine Interrupts nötig
    };

    for (int i = 0; i < NUMBER_OF_PIN; i++) {
        io_conf.pin_bit_mask = 1ULL << pinSteuerkreuz[i];
        gpio_config(&io_conf);
    }


}

void button_task() {
    const TickType_t delay = pdMS_TO_TICKS(20); // Polling alle 20ms

    while (true) {
        const int64_t now = esp_timer_get_time() / 1000; // in ms
        for (int i = 0; i < NUMBER_OF_PIN; i++) {
            const int pin = pinSteuerkreuz[i];

            if ( gpio_get_level(pin) == BUTTON_ACTIVE_LEVEL && (now - last_press_times[i] > DEBOUNCE_MS)) {
                last_press_times[i] = now;
                ESP_LOGI(TAG, "Button %d pressed", pin);
                button_handler_different_games(pin);
            }
        }

        vTaskDelay(delay);

    }
}


void button_handler_different_games(uint8_t pin) {
    if (current_game == GAME_CONNECT4) {
        switch (pin) {
            case LEFT_BTN: move_left(); break;
                case RIGHT_BTN: move_right(); break;
                case DOWN_BTN: drop_piece(); break;
                case UP_BTN: reset_game(); break;
            default: break;
        }

    }
}
void handle_home_button() {
    smiley_animation();
    current_game = (current_game + 1) % GAME_COUNT;
    reset_game();
}

/*
// === Main menu Animation ===

// Gelbes Gesicht zeichnen
static void draw_face(uint8_t r, uint8_t g, uint8_t b) {
    for (int y = 1; y < 7; y++) {
        for (int x = 1; x < 7; x++) {
            if (!((x == 1 && (y == 1 || y == 6)) ||
                  (x == 6 && (y == 1 || y == 6)))) {
                display_set_pixel(x, y, r, g, b);
            }
        }
    }
}

// Lächelnder Mund
static void draw_mouth(uint8_t r, uint8_t g, uint8_t b) {
    display_set_pixel(2, 5, r, g, b);
    display_set_pixel(3, 6, r, g, b);
    display_set_pixel(4, 6, r, g, b);
    display_set_pixel(5, 5, r, g, b);
}

// Normale Augen
static void draw_eyes_normal(uint8_t r, uint8_t g, uint8_t b) {
    display_set_pixel(2, 2, r, g, b);
    display_set_pixel(2, 3, r, g, b);
    display_set_pixel(5, 2, r, g, b);
    display_set_pixel(5, 3, r, g, b);
}

// Zwinker-Auge (rechtes Auge zu)
static void draw_eyes_wink(uint8_t r, uint8_t g, uint8_t b) {
    // Linkes Auge bleibt gleich
    display_set_pixel(2, 2, r, g, b);
    display_set_pixel(2, 3, r, g, b);
    // Rechtes Auge zu (nur ein Strich)
    display_set_pixel(5, 3, r, g, b);
}

void smiley_animation() {
    const uint8_t YR = 255, YG = 255, YB = 0;   // Gelb
    const uint8_t ER = 0,   EG = 0,   EB = 0;   // Schwarz (Augen)
    const uint8_t MR = 255, MG = 0,   MB = 0;   // Rot (Mund)

    for (uint8_t i = 0; i < 2; i++) {
        //Normaler Smiley
        display_clear();
        draw_face(YR, YG, YB);
        draw_eyes_normal(ER, EG, EB);
        draw_mouth(MR, MG, MB);
        display_update();
        vTaskDelay(pdMS_TO_TICKS(1500));

        //Zwinkert
        display_clear();
        draw_face(YR, YG, YB);
        draw_eyes_wink(ER, EG, EB);
        draw_mouth(MR, MG, MB);
        display_update();
        vTaskDelay(pdMS_TO_TICKS(400));

        //wieder normal
        display_clear();
        draw_face(YR, YG, YB);
        draw_eyes_normal(ER, EG, EB);
        draw_mouth(MR, MG, MB);
        display_update();
        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}
*/

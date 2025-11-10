#include <stdio.h>
#include <string.h>
#include <board.h>
#include <display.h>
#include "game_conect4.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/task.h"
#include <portmacro.h>
#include <stdbool.h>
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



// Menü-Zustände
typedef enum {MENU_MODE, MENU_AI} MenuState;
static MenuState menu_state = MENU_MODE;

static bool singleplayer = true;
static uint8_t ai_level = 1; // 1=leicht, 2=mittel, 3=schwer
static bool menu_done = false;

// ============================================================================
// Hilfsfunktionen: Smiley auf Board
// ============================================================================

void draw_smiley_for_ai(uint8_t level) {
    board_clear();

    uint8_t eye_color = 3; // rot Augen
    uint8_t mouth_color = 1; // Mundfarbe

    // Augen
    board_set(2,2,eye_color);
    board_set(5,2,eye_color);

    // Mund je nach Schwierigkeitslevel
    switch(level) {
        case 1: // leicht → lächelnd
            board_set(2,5,mouth_color);
            board_set(3,6,mouth_color);
            board_set(4,6,mouth_color);
            board_set(5,5,mouth_color);
            break;
        case 2: // mittel → neutral
            board_set(2,5,mouth_color);
            board_set(3,5,mouth_color);
            board_set(4,5,mouth_color);
            board_set(5,5,mouth_color);
            break;
        case 3: // schwer → wütend
            board_set(2,6,mouth_color);
            board_set(3,5,mouth_color);
            board_set(4,5,mouth_color);
            board_set(5,6,mouth_color);
            break;
    }

}

void draw_mode_selection(bool singleplayer) {
    board_clear();

    uint8_t color = singleplayer ? 2 : 3; // Grün = Singleplayer (C), Rot = Multiplayer (H)

    if (singleplayer) {
        // --- Buchstabe C ---
        for (int x = 2; x <= 5; x++) board_set(x, 2, color); // obere Linie
        for (int x = 2; x <= 5; x++) board_set(x, 6, color); // untere Linie
        for (int y = 3; y <= 5; y++) board_set(2, y, color); // linke Linie
    } else {
        // --- Buchstabe H ---
        for (int y = 2; y <= 6; y++) board_set(2, y, color); // linke Säule
        for (int y = 2; y <= 6; y++) board_set(5, y, color); // rechte Säule
        for (int x = 2; x <= 5; x++) board_set(x, 4, color); // mittlere Querlinie
    }

}


// ============================================================================
// Buttons auswerten
// ============================================================================

void menu_handle_button(uint8_t pin) {
    if (pin !=0) {
        int64_t now = esp_timer_get_time()/1000; // ms
        if(now - last_press_times[pin] < DEBOUNCE_MS) return;
        last_press_times[pin] = now;
    }
    if(menu_state == MENU_MODE){
        if(pin == LEFT_BTN || pin == RIGHT_BTN) singleplayer = !singleplayer;
        if(pin == DOWN_BTN) {
            menu_state = MENU_AI;
            menu_handle_button(0);
        }else {
            // Weiter zu AI-Level
            draw_mode_selection(singleplayer);
        }
    } else if(menu_state == MENU_AI){
        if (!singleplayer)menu_done=true;
        if(pin == LEFT_BTN && ai_level>1) ai_level--;
        if(pin == RIGHT_BTN && ai_level<3) ai_level++;
        if(pin == DOWN_BTN) {
            menu_done = true;
        }else {
            draw_smiley_for_ai(ai_level);
        }
    }
}

// ============================================================================
// Task zur Menüsteuerung
// ============================================================================

void menu_task(void *pvParameters){
    draw_mode_selection(singleplayer); // erstes Menü anzeigen

    while(!menu_done){
        // Polling alle 50ms
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    set_Data(singleplayer,ai_level);
    board_clear();
    reset_game();
    vTaskDelete(NULL);
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void app_main(void) {

    logic_connect4_init();

    // Button-Task starten
    //17% Stack mehr als du brauchst
    xTaskCreate(button_task, "button_task", 2000, NULL, 7, NULL);

    xTaskCreate(menu_task,"menu_task",4000,NULL,5,NULL);

    xTaskCreate(display_task, "taskDisplay", 4000, NULL, 5, NULL);


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
    const TickType_t delay = pdMS_TO_TICKS(10); // Polling alle 20ms
    buttons_init();

    while (true) {
        const int64_t now = esp_timer_get_time() / 1000; // in ms
        for (int i = 0; i < NUMBER_OF_PIN; i++) {
            const int pin = pinSteuerkreuz[i];

            if (gpio_get_level(pin) == BUTTON_ACTIVE_LEVEL && (now - last_press_times[i] > DEBOUNCE_MS)) {
                last_press_times[i] = now;
                ESP_LOGI(TAG, "Button %d pressed", pin);
                button_handler_different_games(pin);
            }
        }

        vTaskDelay(delay);

    }
}
void start_game() {
    menu_done = false;
    menu_state = MENU_MODE;
    xTaskCreate(menu_task,"menu_task",4000,NULL,5,NULL);
}

void button_handler_different_games(uint8_t pin) {
    if (!menu_done) {
        menu_handle_button(pin);
        return;
    }
    if (current_game == GAME_CONNECT4) {
        switch (pin) {
            case LEFT_BTN: move_left(); break;
            case RIGHT_BTN: move_right(); break;
            case DOWN_BTN: drop_piece(); break;
            case UP_BTN: start_game(); break;
            default: break;
        }

    }
}
void handle_home_button() {
    //smiley_animation();
    current_game = (current_game + 1) % GAME_COUNT;
    reset_game();
}



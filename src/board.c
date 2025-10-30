#define LEFT_BTN   2
#define RIGHT_BTN  3
#define DOWN_BTN   4
#define UP_BTN     5
#define A_BTN      6
#define B_BTN      7
#define HOME_BTN   8
static const uint8_t pinSteuerkreuz[] = {LEFT_BTN, RIGHT_BTN, DOWN_BTN, UP_BTN};
static const uint8_t numberOfPin = sizeof(pinSteuerkreuz) / sizeof(pinSteuerkreuz[0]);
#define BUTTON_ACTIVE_LEVEL 0  // 0 für active-low, 1 für active-high  Der Rest ist schon gecodet in c mit platformio hier ist die Spielinterne Board logik #include "board.h"
#include <stdio.h>  // für printf()
#include  <string.h>
#define TAG "BOARD"

uint8_t board[BOARD_BYTES];

// Hilfsfunktion: Index berechnen (0–63)
static inline uint8_t board_index(uint8_t x, uint8_t y) {
    return y * BOARD_SIZE + x;
}

// Setzt Feldwert (2 Bits)
void board_set( int x, int y, uint8_t value) {
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE) return; // Schutz
    if (value > 3) value = 3; // Maximal 2 Bits zulässig

    uint8_t n = board_index(x, y);
    uint8_t byte_index = n / 4;          // 4 Felder pro Byte
    uint8_t bit_offset = (n % 4) * 2;    // Position im Byte (0, 2, 4, 6)

    board[byte_index] &= ~(0x3 << bit_offset);        // alte Bits löschen
    board[byte_index] |=  (value & 0x3) << bit_offset; // neue Bits setzen
}

// Liest Feldwert
uint8_t board_get(int x, int y) {
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE) return 0;

    uint8_t n = board_index(x, y);
    uint8_t byte_index = n / 4;
    uint8_t bit_offset = (n % 4) * 2;
    return (board[byte_index] >> bit_offset) & 0x3;
}


// Setzt das ganze Brett auf 0
void board_clear() {
    memset(board, 0, sizeof(board));
}

//Nachdem der Code funktioniert löschen
// Gibt Brett im Terminal aus (Debug)
void board_print() {
    for (uint8_t y = 0; y < BOARD_SIZE; y++) {
        for (uint8_t x = 0; x < BOARD_SIZE; x++) {
            printf("%d ", board_get( x, y));
        }
        printf("\n");
    }
}
#include "board.h"
#include <esp_log.h>
#include <stdio.h>  // für printf()
#include  <string.h>
#define TAG "BOARD"

uint8_t board[BOARD_BYTES];


static uint8_t board_index(int x, int y) {
    // Reihenfolge: n = y * BOARD_SIZE + x
    return (uint8_t)(y * BOARD_SIZE + x);
}



// Setzt Feldwert (2 Bits)
void board_set( int x, int y, uint8_t value) {
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE) {
        ESP_LOGI(TAG, "Korrdinaten größer als acht ", " x:",x, " y:", y);
        return;// Schutz
    }
    if (value > 3) value = 3; // Maximal 2 Bits zulässig

    uint8_t n = board_index(x, y);
    uint8_t byte_index = n / 4;          // 4 Felder pro Byte
    uint8_t bit_offset = (n % 4) * 2;    // Position im Byte (0, 2, 4, 6)

    board[byte_index] &= ~(0x3 << bit_offset);        // alte Bits löschen
    board[byte_index] |=  (value & 0x3) << bit_offset; // neue Bits setze
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

uint8_t* get_Board(void) {
    return board;
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

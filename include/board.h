#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

// --- Konstanten ---
#define BOARD_SIZE 8           // 8x8 Felder
#define BOARD_FIELDS (BOARD_SIZE * BOARD_SIZE)  // 64
#define BOARD_BYTES (BOARD_FIELDS / 4)          // 16 Bytes (2 Bits pro Feld)



// Setzt ein Feld (0–3) an Position (x, y)
void board_set( int x, int y, uint8_t value);

// Liest den Zustand eines Feldes (0–3)
uint8_t board_get( int x, int y);

// Setzt das gesamte Brett auf 0
void board_clear();

// Gibt das Brett in der Konsole aus (nur zum Debuggen)
void board_print();

uint8_t* get_Board();

#endif

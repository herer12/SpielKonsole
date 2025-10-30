#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "board.h"
#include "game_conect4.h"

#define ROWS 6
#define COLS 7
#define TAG "game_connect4"

static bool game_over = false;
static uint8_t col = 3;
static uint8_t player = 1;

// ===== Prototypen =====
static bool drop_piece_in_col(uint8_t c);
static int ai_choose_column();
static bool would_win(uint8_t test_player, uint8_t test_col);

// ------------------------------------------------------------
// Initialisierung
// ------------------------------------------------------------
void logic_connect4_init() {
    board_clear();
    srand(time(NULL));
    player = 1;
    col = 3;
    game_over = false;
    board_set(col, 0, player);
}

// ------------------------------------------------------------
// Spielsteuerung
// ------------------------------------------------------------
void move_left() {
    if (game_over) return;
    board_set(col, 0, 0);
    if (col > 0) col--;
    board_set(col, 0, player);
    board_print();
}

void move_right() {
    if (game_over) return;
    board_set(col, 0, 0);
    if (col < COLS - 1) col++;
    board_set(col, 0, player);
    board_print();
}

void drop_piece() {
    if (game_over) return;

    if (!drop_piece_in_col(col)) {
        ESP_LOGI(TAG, "Spalte %d ist voll", col);
        return;
    }

    if (check_win()) {
        ESP_LOGI(TAG, "Player %d gewinnt!", player);
        game_over = true;
        return;
    }

if (SINGLEPLAYER == 1) {
    player = 2;
    int ai_col = ai_choose_column();
    ESP_LOGI(TAG, "KI (Level %d) wählt Spalte %d", AI_LEVEL, ai_col);

    drop_piece_in_col(ai_col);

    if (check_win()) {
        ESP_LOGI(TAG, "KI (Player %d) gewinnt!", player);
        game_over = true;
        return;
    }

    player = 1;
    col = 3;
    board_set(col, 0, player);
}else {
    player = (player == 1) ? 2 : 1;
    col = 3;
    board_set(col, 0, player);
}


    board_print();
}

// ------------------------------------------------------------
// Spiel-Reset
// ------------------------------------------------------------
void reset_game() {
    board_clear();
    player = 1;
    col = 3;
    game_over = false;
    board_set(col, 0, player);
    ESP_LOGI(TAG, "Spiel zurückgesetzt");
}

// ------------------------------------------------------------
// Hilfsfunktionen
// ------------------------------------------------------------
static bool drop_piece_in_col(uint8_t c) {
    for (int y = ROWS - 1; y >= 1; y--) {
        if (board_get(c, y) == 0) {
            board_set(c, y, player);
            board_set(col, 0, 0);
            return true;
        }
    }
    return false;
}

// Prüft, ob Spieler in Spalte gewinnen würde
static bool would_win(uint8_t test_player, uint8_t test_col) {
    // Temporär: Finde erste freie Position
    for (int y = ROWS - 1; y >= 1; y--) {
        if (board_get(test_col, y) == 0) {
            board_set(test_col, y, test_player);
            bool win = check_win();
            board_set(test_col, y, 0);  // Rückgängig machen
            return win;
        }
    }
    return false;
}

// KI-Auswahl nach Level
static int ai_choose_column() {
    int valid_cols[COLS];
    int valid_count = 0;

    // Sammle freie Spalten
    for (int c = 0; c < COLS; c++) {
        if (board_get(c, 1) == 0)
            valid_cols[valid_count++] = c;
    }
    if (valid_count == 0) {
        ESP_LOGI(TAG, "Unentschieden – keine gültigen Züge mehr.");
        game_over = true;
        return 0;
    }

    // 2KI-Logik nach Level
    if (AI_LEVEL >= 3) {
        // Schwer: Versuch zu gewinnen
        for (int i = 0; i < valid_count; i++) {
            int c = valid_cols[i];
            if (would_win(2, c)) {
                ESP_LOGI(TAG, "KI sieht Gewinn in Spalte %d", c);
                return c;
            }
        }
    }

    if (AI_LEVEL >= 2) {
        // Mittel+: Blockiere, wenn Spieler kurz vor Sieg steht
        for (int i = 0; i < valid_count; i++) {
            int c = valid_cols[i];
            if (would_win(1, c)) {
                ESP_LOGI(TAG, "KI blockt Spalte %d", c);
                return c;
            }
        }
    }

    // Leicht (oder fallback): Zufall
    int choice = rand() % valid_count;
    return valid_cols[choice];
}

// ------------------------------------------------------------
// Gewinnprüfung
// ------------------------------------------------------------
bool check_win() {
    const int dirs[4][2] = {
        {1, 0},  // →
        {0, 1},  // ↓
        {1, 1},  // ↘
        {-1, 1}  // ↙
    };

    for (int d = 0; d < 4; d++) {
        const int dx = dirs[d][0];
        const int dy = dirs[d][1];
        for (int x = 0; x < COLS; x++) {
            for (int y = 1; y < ROWS; y++) {
                int count = 0;
                for (int k = 0; k < 4; k++) {
                    int nx = x + k * dx;
                    int ny = y + k * dy;
                    if (nx < 0 || nx >= COLS || ny < 1 || ny >= ROWS) break;
                    if (board_get(nx, ny) == player) count++;
                    else break;
                }
                if (count == 4) return true;
            }
        }
    }
    return false;
}
#include <esp_log.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "board.h"
#include "game_conect4.h"

#define ROWS 7
#define COLS 7
#define TAG "game_connect4"

bool singleplayer = true;
uint8_t ai_level= 1 ; //1 leicht, 2 Forgeschritten 3 Schwer(perfektes Spiel)
bool game_over = false;
uint8_t col = 3;
uint8_t player = 1;
void ai_move_task(void *pvParameters);
void set_Data(bool singleplayerMode, uint8_t kiLevel) {
    if (!singleplayerMode) {
        singleplayer = false;
    }else {
        singleplayer = true;
    }
    ai_level = kiLevel;
}

// ------------------------------------------------------------
// Initialisierung
// ------------------------------------------------------------
void logic_connect4_init() {
    srand(time(NULL));
    reset_game();
}

// ------------------------------------------------------------
// Spielsteuerung
// ------------------------------------------------------------
void move_left() {
    if (game_over) return;
    if (singleplayer&&player==2) {

    }else {
        board_set(col, 0, 0);
    }
    if (col > 0) {
        col--;
    } else {
        col = COLS - 1;
    }
    board_set(col, 0, player);
}

void move_right() {
    if (game_over) return;
    if (singleplayer&&player==2) {

    }else {
        board_set(col, 0, 0);
    }
    if (col < COLS - 1) {
        col++;
    } else {
        col = 0;
    }
    board_set(col, 0, player);
}

void drop_piece() {
    if (game_over) return;

    if (!drop_piece_in_col(col)) {
        return;
    }

    if (check_win_for_player(player)) {
        game_over = true;
        reset_game();
        return;
    }

    if (singleplayer) {
        // KI-Zug wird als eigener Task gestartet
        player = 2;
        board_set(col, 0, 0); // Anzeige löschen

        BaseType_t res = xTaskCreate(
            ai_move_task,     // Funktion
            "ai_move_task",   // Taskname
            8192,             // Stackgröße (bei Bedarf größer)
            NULL,             // Parameter
            4,                // Priorität (niedriger als Display!)
            NULL
        );

        if (res != pdPASS) {
            ESP_LOGE(TAG, "AI-Task konnte nicht erstellt werden! Fallback: synchron");
            int ai_col = ai_choose_column();
            vTaskDelay(pdMS_TO_TICKS(300));
            drop_piece_in_col(ai_col);
            if (check_win_for_player(2)) game_over = true;
            player = 1;
            col = 3;
        }

    } else {
        // Multiplayer
        player = (player == 1) ? 2 : 1;
        col = 3;
        board_set(col, 0, player);
    }

    board_set(col, 0, player);

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
}


// ------------------------------------------------------------
// Hilfsfunktionen
// ------------------------------------------------------------
bool drop_piece_in_col(uint8_t c) {
    for (int y = ROWS - 1; y >= 1; y--) { // y = 6..1
        if (board_get(c, y) == 0) {
            board_set(c, y, player);
            board_set(c, 0, 0); // Anzeige in SPALTE c löschen (NICHT global col)
            return true;
        }
    }
    return false; // Spalte voll
}


// ------------------------------------------------------------
// Gewinnprüfung für beliebigen Spieler
bool check_win_for_player(uint8_t test_player) {
    const int directionsOfWin[4][2] = {
        {1,0}, {0,1}, {1,1}, {-1,1}
    };
    for(int numberDirections=0; numberDirections<4; numberDirections++){
        uint8_t directionOfWinForXDirection = directionsOfWin[numberDirections][0];
        uint8_t directionOfWinForYDirection = directionsOfWin[numberDirections][1];
        for(int x=0; x<COLS; x++){
            for(int y=1; y<ROWS; y++){
                int count=0;
                for(int k=0; k<4; k++){
                    int nextX=x+k*directionOfWinForXDirection;
                    int nextY=y+k*directionOfWinForYDirection;
                    if(nextX<0 || nextX>=COLS || nextY<1 || nextY>=ROWS) break;
                    if(board_get(nextX,nextY)==test_player) count++;
                    else break;
                }
                if(count==4) return true;
            }
        }
    }
    return false;
}

// ------------------------------------------------------------
// Heuristische Bewertungsfunktion
// ------------------------------------------------------------
static int evaluate_board(uint8_t board[ROWS][COLS]) {
    int score = 0;
    const int dirs[4][2] = {{1,0},{0,1},{1,1},{-1,1}};

    for (int y = 1; y < ROWS; y++) {
        for (int x = 0; x < COLS; x++) {
            if (board[y][x] == 0) continue;
            uint8_t p = board[y][x];
            for (int d = 0; d < 4; d++) {
                int dx = dirs[d][0], dy = dirs[d][1];
                int count = 0, empty = 0;
                for (int k = 0; k < 4; k++) {
                    int nx = x + k * dx, ny = y + k * dy;
                    if (nx < 0 || nx >= COLS || ny < 1 || ny >= ROWS) break;
                    if (board[ny][nx] == p) count++;
                    else if (board[ny][nx] == 0) empty++;
                    else break;
                }

                if (count == 4) score += (p == 2 ? 10000 : -10000);
                else if (count == 3 && empty == 1) score += (p == 2 ? 100 : -100);
                else if (count == 2 && empty == 2) score += (p == 2 ? 10 : -10);
            }
        }
    }
    return score;
}

// ------------------------------------------------------------
// Modifizierte Minimax-Funktion mit Alpha-Beta-Pruning
// ------------------------------------------------------------
static int minimax_ab(uint8_t board[ROWS][COLS], int depth, int alpha, int beta, bool maximizing) {
    // Abbruchbedingungen
    if (check_win_for_player(2)) return 10000 + depth;  // schneller Sieg ist besser
    if (check_win_for_player(1)) return -10000 - depth; // schneller Verlust ist schlechter
    if (depth == 0) return evaluate_board(board);

    int bestScore = maximizing ? INT_MIN : INT_MAX;
    uint8_t current_player = maximizing ? 2 : 1;

    for (int c = 0; c < COLS; c++) {
        int r = -1;
        for (int y = ROWS - 1; y >= 1; y--) {
            if (board[y][c] == 0) { r = y; break; }
        }
        if (r == -1) continue;

        board[r][c] = current_player;
        int score = minimax_ab(board, depth - 1, alpha, beta, !maximizing);
        board[r][c] = 0;

        if (maximizing) {
            if (score > bestScore) bestScore = score;
            if (bestScore > alpha) alpha = bestScore;
        } else {
            if (score < bestScore) bestScore = score;
            if (bestScore < beta) beta = bestScore;
        }

        // Alpha-Beta-Abbruch
        if (beta <= alpha) break;

        // CPU/WDT entlasten
        if ((depth & 1) == 0) vTaskDelay(pdMS_TO_TICKS(1));
    }

    return bestScore;
}

// ------------------------------------------------------------
// KI-Funktion mit Alpha-Beta-Minimax
// ------------------------------------------------------------
int ai_choose_column() {
    int valid_cols[COLS], valid_count = 0;
    for (int c = 0; c < COLS; c++)
        if (board_get(c, 1) == 0) valid_cols[valid_count++] = c;
    if (valid_count == 0) { game_over = true; return 0; }

    // Level 1: einfache, schnelle Logik
    if (ai_level == 1) {
        for (int i = 0; i < valid_count; i++) {
            int c = valid_cols[i];
            for (int y = ROWS - 1; y >= 1; y--) {
                if (board_get(c, y) == 0) {
                    board_set(c, y, 2);
                    if (check_win_for_player(2)) { board_set(c, y, 0); return c; }
                    board_set(c, y, 0);
                    break;
                }
            }
        }
        return valid_cols[rand() % valid_count];
    }

    // Level 2–3: heuristisch
    int depth = (ai_level == 2) ? 4 : 6;

    int bestScore = INT_MIN;
    int bestCol = valid_cols[0];
    uint8_t temp_board[ROWS][COLS];

    for (int y = 1; y < ROWS; y++) {
        for (int x = 0; x < COLS; x++)
            temp_board[y][x] = board_get(x, y);
    }

    for (int i = 0; i < valid_count; i++) {
        int c = valid_cols[i];
        int r = -1;
        for (int y = ROWS - 1; y >= 1; y--) {
            if (temp_board[y][c] == 0) { r = y; break; }
        }
        if (r == -1) continue;

        temp_board[r][c] = 2;
        int score = minimax_ab(temp_board, depth - 1, INT_MIN, INT_MAX, false);
        temp_board[r][c] = 0;

        if (score > bestScore) {
            bestScore = score;
            bestCol = c;
        }

        // Kurze Pause, um WDT zu beruhigen
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    ESP_LOGI(TAG, "KI (AlphaBeta, Level %d) wählt Spalte %d mit Score %d", ai_level, bestCol, bestScore);
    return bestCol;
}


void ai_move_task(void *pvParameters) {
    (void) pvParameters;

    // kleine Pause, um Display weiterlaufen zu lassen
    vTaskDelay(pdMS_TO_TICKS(50));

    // KI-Spalte berechnen
    int ai_col = ai_choose_column();

    // kurze Wartezeit zur "Animation"
    vTaskDelay(pdMS_TO_TICKS(100));

    // Zug durchführen
    if (!drop_piece_in_col(ai_col)) {
        ESP_LOGW("AI", "Spalte %d ist voll!", ai_col);
    } else if (check_win_for_player(2)) {
        game_over = true;
        ESP_LOGI("AI", "KI gewinnt!");
        // Optional: reset_game();
    }

    // Spieler zurücksetzen
    player = 1;
    col = 3;
    board_set(col, 0, player);

    // Task beenden
    vTaskDelete(NULL);
}

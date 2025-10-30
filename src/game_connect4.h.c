#include <esp_log.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <display.h>

#include "board.h"
#include "game_conect4.h"

#define ROWS 6
#define COLS 7
#define TAG "game_connect4"

static bool game_over = false;
static uint8_t col = 3;
static uint8_t player = 1;

static bool drop_piece_in_col(uint8_t c);
static int ai_choose_column();
static bool check_win_for_player(uint8_t test_player);


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

    if (check_win_for_player(player)) {
        ESP_LOGI(TAG, "Player %d gewinnt!", player);
        game_over = true;
        return;
    }

    if (SINGLEPLAYER == 1) {
        // KI-Zug
        player = 2;
        int ai_col = ai_choose_column();
        ESP_LOGI(TAG, "KI (Level %d) wählt Spalte %d", AI_LEVEL, ai_col);
        vTaskDelay(pdMS_TO_TICKS(300));
        drop_piece_in_col(ai_col);

        if (check_win_for_player(player)) {
            ESP_LOGI(TAG, "KI (Player %d) gewinnt!", player);
            game_over = true;
            return;
        }

        player = 1;
        col = 3;
    } else {
        // Multiplayer
        player = (player == 1) ? 2 : 1;
        col = 3;

    }
    board_set(col, 0, player);

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

// ------------------------------------------------------------
// Gewinnprüfung für beliebigen Spieler
static bool check_win_for_player(uint8_t test_player) {
    const int dirs[4][2] = {
        {1,0}, {0,1}, {1,1}, {-1,1}
    };
    for(int d=0; d<4; d++){
        int dx = dirs[d][0], dy = dirs[d][1];
        for(int x=0; x<COLS; x++){
            for(int y=1; y<ROWS; y++){
                int count=0;
                for(int k=0; k<4; k++){
                    int nx=x+k*dx, ny=y+k*dy;
                    if(nx<0 || nx>=COLS || ny<1 || ny>=ROWS) break;
                    if(board_get(nx,ny)==test_player) count++;
                    else break;
                }
                if(count==4) return true;
            }
        }
    }
    return false;
}

// ------------------------------------------------------------
// MiniMax Funktion für KI (Level 4/5)
static int minimax(uint8_t temp_board[ROWS][COLS], int depth, bool maximizingPlayer) {
    if(depth==0) return 0;

    int bestScore = maximizingPlayer ? INT_MIN : INT_MAX;
    uint8_t player_to_check = maximizingPlayer ? 2 : 1;

    for(int c=0;c<COLS;c++){
        int r=-1;
        for(int y=ROWS-1;y>=1;y--) if(temp_board[y][c]==0){ r=y; break; }
        if(r==-1) continue;

        temp_board[r][c] = player_to_check;
        int score;
        if(check_win_for_player(player_to_check)){
            score = maximizingPlayer ? 10 : -10;
        } else {
            score = minimax(temp_board, depth-1, !maximizingPlayer);
        }
        temp_board[r][c] = 0;

        if(maximizingPlayer && score>bestScore) bestScore=score;
        if(!maximizingPlayer && score<bestScore) bestScore=score;
    }
    return bestScore;
}

// ------------------------------------------------------------
// KI-Funktion für alle Levels
int ai_choose_column() {
    int valid_cols[COLS], valid_count=0;

    // Freie Spalten sammeln
    for(int c=0;c<COLS;c++)
        if(board_get(c,1)==0) valid_cols[valid_count++]=c;
    if(valid_count==0){ game_over=true; return 0; }

    //
    if(AI_LEVEL <=3){
        // Level 3: Gewinnchance prüfen
        if(AI_LEVEL>=3){
            for(int i=0;i<valid_count;i++){
                int c=valid_cols[i];
                for(int y=ROWS-1;y>=1;y--){
                    if(board_get(c,y)==0){
                        board_set(c,y,2);
                        if(check_win_for_player(2)){ board_set(c,y,0); return c; }
                        board_set(c,y,0);
                        break;
                    }
                }
            }
        }
        // Level 2: Blockieren Spieler
        if(AI_LEVEL>=2){
            for(int i=0;i<valid_count;i++){
                int c=valid_cols[i];
                for(int y=ROWS-1;y>=1;y--){
                    if(board_get(c,y)==0){
                        board_set(c,y,1);
                        if(check_win_for_player(1)){ board_set(c,y,0); return c; }
                        board_set(c,y,0);
                        break;
                    }
                }
            }
        }
        // Leicht: Zufall
        return valid_cols[rand() % valid_count];
    }

    // Level 4/5: MiniMax / Perfect Play
    int bestScore=INT_MIN, bestCol=valid_cols[0];
    uint8_t temp_board[ROWS][COLS];
    for(int y=0;y<ROWS;y++)
        for(int x=0;x<COLS;x++)
            temp_board[y][x] = board_get(x,y);

    int depth = (AI_LEVEL==4) ? 3 : 6;
    for(int i=0;i<valid_count;i++){
        int c = valid_cols[i];
        int r=-1;
        for(int y=ROWS-1;y>=1;y--) if(temp_board[y][c]==0){r=y; break;}
        if(r==-1) continue;

        temp_board[r][c] = 2;
        int score = minimax(temp_board, depth-1, false);
        temp_board[r][c] = 0;

        if(score>bestScore){ bestScore=score; bestCol=c; }
    }
    return bestCol;
}

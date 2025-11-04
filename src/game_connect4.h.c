#include <esp_log.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "board.h"
#include "game_conect4.h"

#define ROWS 6
#define COLS 7
#define TAG "game_connect4"

bool singleplayer = true;
uint8_t ai_level= 1 ; //1 leicht, 2 Forgeschritten 3 Schwer(perfektes Spiel)
bool game_over = false;
uint8_t col = 3;
uint8_t player = 1;

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

    if (singleplayer) {
        // KI-Zug
        player = 2;
        int ai_col = ai_choose_column();
        ESP_LOGI(TAG, "KI (Level %d) wählt Spalte %d", ai_level, ai_col);
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
bool drop_piece_in_col(uint8_t c) {
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
                    uint8_t nextX=x+k*directionOfWinForXDirection;
                    uint8_t nextY=y+k*directionOfWinForYDirection;
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
    if(ai_level ==1){
        // Level 1: Gewinnchance prüfen, Blockieren Spieler, Zufall

            for(int i=0;i<valid_count;i++){
                int c=valid_cols[i];
                for(int y=ROWS-1;y>=1;y--){
                    if(board_get(c,y)==0){
                        board_set(c,y,2);
                        if(check_win_for_player(2)){ board_set(c,y,0); return c; }
                        if(check_win_for_player(1)){ board_set(c,y,0); return c; }
                        board_set(c,y,0);
                        break;
                    }
                }
            }
        // Leicht:
        return valid_cols[rand() % valid_count];
    }

    // Level 4/5: MiniMax / Perfect Play
    int bestScore=INT_MIN, bestCol=valid_cols[0];
    uint8_t temp_board[ROWS][COLS];
    for(int y=0;y<ROWS;y++)
        for(int x=0;x<COLS;x++)
            temp_board[y][x] = board_get(x,y);

    int depth = (ai_level==2) ? 3 : 6;
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

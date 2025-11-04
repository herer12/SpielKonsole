#ifndef GAME_CONNECT4_H
#define GAME_CONNECT4_H

#include <stdbool.h>

     // 1 = gegen KI, 0 = 2-Spieler
      // 1 = leicht, 2 = mittel, 3 = schwer

void logic_connect4_init();
void move_left();
void move_right();
void drop_piece();
void reset_game();
bool check_win();
bool drop_piece_in_col(uint8_t c);
int ai_choose_column();
bool check_win_for_player(uint8_t test_player);


#endif // GAME_CONNECT4_H
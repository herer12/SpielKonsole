#ifndef GAME_CONNECT4_H
#define GAME_CONNECT4_H

#include <stdbool.h>

#define SINGLEPLAYER 1     // 1 = gegen KI, 0 = 2-Spieler
#define AI_LEVEL (3)         // 1 = leicht, 2 = mittel, 3 = schwer

void logic_connect4_init();
void move_left();
void move_right();
void drop_piece();
void reset_game();
bool check_win();


#endif // GAME_CONNECT4_H
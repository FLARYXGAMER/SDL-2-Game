#ifndef PLAYER_H
#define PLAYER_H

#include <SDL2/SDL.h>

/* ADT 1: Player
   The struct is hidden in player.c. Other files must use these functions. */
typedef struct Player Player;

Player *playerCreate(void);
void playerDestroy(Player *player);
void playerReset(Player *player);
void playerHandleInput(Player *player, const Uint8 *keys);
const SDL_Rect *playerGetRect(const Player *player);
SDL_Rect *playerGetMutableRect(Player *player);
int playerGetGunX(const Player *player, int gunIndex);
int playerGetY(const Player *player);

#endif

#include "player.h"
#include "constants.h"
#include <stdlib.h>

struct Player {
    SDL_Rect rect;
};

Player *playerCreate(void)
{
    Player *player = malloc(sizeof(Player));
    if (player != NULL)
        playerReset(player);
    return player;
}

void playerDestroy(Player *player)
{
    free(player);
}

void playerReset(Player *player)
{
    if (player == NULL)
        return;

    player->rect.x = SCREEN_WIDTH / 2 - 32;
    player->rect.y = SCREEN_HEIGHT - 80;
    player->rect.w = 64;
    player->rect.h = 64;
}

void playerHandleInput(Player *player, const Uint8 *keys)
{
    if (player == NULL || keys == NULL)
        return;

    if (keys[SDL_SCANCODE_LEFT])  player->rect.x -= 10;
    if (keys[SDL_SCANCODE_RIGHT]) player->rect.x += 10;
    if (keys[SDL_SCANCODE_UP])    player->rect.y -= 10;
    if (keys[SDL_SCANCODE_DOWN])  player->rect.y += 10;

    if (player->rect.x < 0) player->rect.x = 0;
    if (player->rect.y < 0) player->rect.y = 0;
    if (player->rect.x > SCREEN_WIDTH  - player->rect.w) player->rect.x = SCREEN_WIDTH  - player->rect.w;
    if (player->rect.y > SCREEN_HEIGHT - player->rect.h) player->rect.y = SCREEN_HEIGHT - player->rect.h;
}

const SDL_Rect *playerGetRect(const Player *player)
{
    return player != NULL ? &player->rect : NULL;
}

SDL_Rect *playerGetMutableRect(Player *player)
{
    return player != NULL ? &player->rect : NULL;
}

int playerGetGunX(const Player *player, int gunIndex)
{
    if (player == NULL)
        return 0;

    if (gunIndex == 0)
        return player->rect.x + player->rect.w / 4 - 8;

    return player->rect.x + 3 * player->rect.w / 4 - 8;
}

int playerGetY(const Player *player)
{
    return player != NULL ? player->rect.y : 0;
}

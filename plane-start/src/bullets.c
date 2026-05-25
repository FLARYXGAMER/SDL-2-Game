#include "bullets.h"
#include "player.h"
#include <stdlib.h>
#include <string.h>

typedef struct
{
    float x;
    float y;
    bool active;
} Bullet;

struct Bullets
{
    Bullet items[MAX_BULLETS];
};

Bullets *bulletsCreate(void)
{
    Bullets *bullets = malloc(sizeof(Bullets));
    if (bullets != NULL)
        bulletsReset(bullets);
    return bullets;
}

void bulletsDestroy(Bullets *bullets)
{
    free(bullets);
}

void bulletsReset(Bullets *bullets)
{
    if (bullets == NULL)
        return;

    memset(bullets->items, 0, sizeof(bullets->items));
}

void bulletsAdd(Bullets *bullets, float x, float y)
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (!bullets->items[i].active)
        {
            bullets->items[i].x = x;
            bullets->items[i].y = y;
            bullets->items[i].active = true;
            break;
        }
    }
}

void bulletsShootFromPlayer(Bullets *bullets, const Player *player)
{
    if (bullets == NULL || player == NULL)
        return;

    bulletsAdd(bullets, playerGetGunX(player, 0), playerGetY(player));
    bulletsAdd(bullets, playerGetGunX(player, 1), playerGetY(player));
}

void bulletsUpdate(Bullets *bullets)
{
    if (bullets == NULL)
        return;

    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (!bullets->items[i].active)
            continue;

        bullets->items[i].y -= 8;
        if (bullets->items[i].y < 0)
            bullets->items[i].active = false;
    }
}

void bulletsRender(const Bullets *bullets, SDL_Renderer *renderer, SDL_Texture *texture)
{
    if (bullets == NULL)
        return;

    SDL_Rect r = {0, 0, 36, 14};

    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (!bullets->items[i].active)
            continue;

        r.x = (int)bullets->items[i].x;
        r.y = (int)bullets->items[i].y;
        SDL_RenderCopyEx(renderer, texture, NULL, &r, -90, NULL, SDL_FLIP_NONE);
    }
}

bool bulletsIsActive(const Bullets *bullets, int index)
{
    if (bullets == NULL || index < 0 || index >= MAX_BULLETS)
        return false;

    return bullets->items[index].active;
}

void bulletsDeactivate(Bullets *bullets, int index)
{
    if (bullets == NULL || index < 0 || index >= MAX_BULLETS)
        return;

    bullets->items[index].active = false;
}

SDL_Rect bulletsGetRect(const Bullets *bullets, int index)
{
    SDL_Rect r = {0, 0, 36, 14};
    if (bullets == NULL || index < 0 || index >= MAX_BULLETS)
        return r;

    r.x = (int)bullets->items[index].x;
    r.y = (int)bullets->items[index].y;
    return r;
}

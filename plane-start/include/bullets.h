#ifndef BULLETS_H
#define BULLETS_H

#include <SDL2/SDL.h>
#include <stdbool.h>

#define MAX_BULLETS 100

/* ADT 2: Bullets*/
typedef struct Bullets Bullets;

Bullets *bulletsCreate(void);
void bulletsDestroy(Bullets *bullets);

void bulletsReset(Bullets *bullets);
void bulletsAdd(Bullets *bullets, float x, float y);
void bulletsUpdate(Bullets *bullets);
void bulletsRender(const Bullets *bullets, SDL_Renderer *renderer, SDL_Texture *texture);

bool bulletsIsActive(const Bullets *bullets, int index);
void bulletsDeactivate(Bullets *bullets, int index);
SDL_Rect bulletsGetRect(const Bullets *bullets, int index);

#endif

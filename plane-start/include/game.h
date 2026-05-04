#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include "sound.h"

#define MAX_BULLETS 100
#define MAX_ENEMIES 20

typedef struct {
    float x, y;
    bool active;
} Bullet;

typedef struct {
    float x, y;
    float baseX;
    float offset;
    int active;
} Enemy;

typedef struct {
    SDL_Texture *map;
    SDL_Texture *plane1;
    SDL_Texture *plane2;
    SDL_Texture *bullet;
    SDL_Texture *enemy;
    SDL_Texture *heart;
    SDL_Texture *button;
} GameTextures;

typedef enum {
    STATE_MAIN_MENU,
    STATE_PLAYING,
    STATE_PAUSE_MENU,
    STATE_GAME_OVER,
    STATE_QUIT
} GameState;

typedef enum {
    MODE_LOCAL,
    MODE_CLIENT
} PlayMode;

SDL_Texture *loadTexture(SDL_Renderer *renderer, const char *path);

void loadGameTextures(SDL_Renderer *renderer, GameTextures *tex);
void destroyGameTextures(GameTextures *tex);

void renderScrollingBackground(SDL_Renderer *renderer, SDL_Texture *mapTex, float mapY);

void handleKeyDown(SDL_Event *event, GameState *state);

void resetLocalGame(SDL_Rect *plane, Bullet bullets[], Enemy enemies[], int *lives, int *score, int *shootTimer);

void runLocalMode(
    SDL_Renderer *renderer, TTF_Font *font,
    const GameTextures *tex, const GameSounds *sounds,
    SDL_Rect *plane, Bullet bullets[], Enemy enemies[],
    int *shootTimer, int shootDelay,
    int *lives, int *score, int *finalScore,
    GameState *state);

void runClientMode(
    SDL_Renderer *renderer, TTF_Font *font,
    const GameTextures *tex,
    GameState *state, PlayMode *mode);

#endif

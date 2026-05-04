#include "game.h"
#include "UI.h"
#include "constants.h"
#include "network.h"
#include <SDL2/SDL_image.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SDL_Texture *loadTexture(SDL_Renderer *renderer, const char *path)
{
    SDL_Surface *surf = IMG_Load(path);
    if (!surf) {
        printf("Failed to load %s: %s\n", path, IMG_GetError());
        return NULL;
    }
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);
    return tex;
}

void loadGameTextures(SDL_Renderer *renderer, GameTextures *tex)
{
    tex->map    = loadTexture(renderer, "resources/map.png");
    tex->plane1 = loadTexture(renderer, "resources/plane1.png");
    tex->plane2 = loadTexture(renderer, "resources/plane2.png");
    tex->bullet = loadTexture(renderer, "resources/test-bullet.png");
    tex->enemy  = loadTexture(renderer, "resources/plane5.png");
    tex->heart  = loadTexture(renderer, "resources/heart.png");
    tex->button = loadTexture(renderer, "resources/buttonPictures/Button.png");
    SDL_SetTextureBlendMode(tex->button, SDL_BLENDMODE_BLEND);
}

void destroyGameTextures(GameTextures *tex)
{
    SDL_DestroyTexture(tex->map);
    SDL_DestroyTexture(tex->plane1);
    SDL_DestroyTexture(tex->plane2);
    SDL_DestroyTexture(tex->bullet);
    SDL_DestroyTexture(tex->enemy);
    SDL_DestroyTexture(tex->heart);
    SDL_DestroyTexture(tex->button);
}

void renderScrollingBackground(SDL_Renderer *renderer, SDL_Texture *mapTex, float mapY)
{
    SDL_Rect r1 = {0, (int)mapY,                 SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_Rect r2 = {0, (int)mapY - SCREEN_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_RenderCopy(renderer, mapTex, NULL, &r1);
    SDL_RenderCopy(renderer, mapTex, NULL, &r2);
}

void handleKeyDown(SDL_Event *event, GameState *state)
{
    if (event->key.repeat)
        return;
    if (event->key.keysym.sym == SDLK_ESCAPE) {
        if (*state == STATE_PLAYING)
            *state = STATE_PAUSE_MENU;
        else if (*state == STATE_PAUSE_MENU)
            *state = STATE_PLAYING;
    }
}

void resetLocalGame(SDL_Rect *plane, Bullet bullets[], Enemy enemies[], int *lives, int *score, int *shootTimer)
{
    plane->x = SCREEN_WIDTH / 2 - 32;
    plane->y = SCREEN_HEIGHT - 80;
    memset(bullets, 0, sizeof(Bullet) * MAX_BULLETS);
    memset(enemies, 0, sizeof(Enemy) * MAX_ENEMIES);
    *lives = PLAYER_LIVES;
    *score = 0;
    *shootTimer = 0;
}

void runLocalMode(
    SDL_Renderer *renderer, TTF_Font *font,
    const GameTextures *tex, const GameSounds *sounds,
    SDL_Rect *plane, Bullet bullets[], Enemy enemies[],
    int *shootTimer, int shootDelay,
    int *lives, int *score, int *finalScore,
    GameState *state)
{
    const Uint8 *keys = SDL_GetKeyboardState(NULL);

    if (keys[SDL_SCANCODE_LEFT])  plane->x -= 5;
    if (keys[SDL_SCANCODE_RIGHT]) plane->x += 5;
    if (keys[SDL_SCANCODE_UP])    plane->y -= 5;
    if (keys[SDL_SCANCODE_DOWN])  plane->y += 5;

    if (plane->x < 0)                        plane->x = 0;
    if (plane->y < 0)                        plane->y = 0;
    if (plane->x > SCREEN_WIDTH  - plane->w) plane->x = SCREEN_WIDTH  - plane->w;
    if (plane->y > SCREEN_HEIGHT - plane->h) plane->y = SCREEN_HEIGHT - plane->h;

    (*shootTimer)++;
    if (*shootTimer >= shootDelay) {
        *shootTimer = 0;
        playSound(sounds->shoot);

        int gunX[2] = {plane->x + plane->w / 4 - 8, plane->x + 3 * plane->w / 4 - 8};
        for (int g = 0; g < 2; g++) {
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (!bullets[i].active) {
                    bullets[i].x      = gunX[g];
                    bullets[i].y      = plane->y;
                    bullets[i].active = 1;
                    break;
                }
            }
        }
    }

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active)
            continue;
        bullets[i].y -= 8;
        if (bullets[i].y < 0)
            bullets[i].active = 0;
    }

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active && rand() % 1000 < 5) {
            enemies[i].baseX  = rand() % (SCREEN_WIDTH - 64);
            enemies[i].x      = enemies[i].baseX;
            enemies[i].y      = -64;
            enemies[i].offset = (float)(rand() % 360) / 50.0f;
            enemies[i].active = 1;
        }
        if (enemies[i].active) {
            enemies[i].y += 2;
            enemies[i].offset += 0.05f;
            enemies[i].x = enemies[i].baseX + 50 * sin(enemies[i].offset);
            if (enemies[i].y > SCREEN_HEIGHT)
                enemies[i].active = 0;
        }
    }

    for (int i = 0; i < MAX_BULLETS; i++) {
        for (int j = 0; j < MAX_ENEMIES; j++) {
            if (!bullets[i].active || !enemies[j].active)
                continue;
            SDL_Rect br = {(int)bullets[i].x, (int)bullets[i].y, 16, 16};
            SDL_Rect en = {(int)enemies[j].x,  (int)enemies[j].y, 64, 64};
            if (SDL_HasIntersection(&br, &en)) {
                bullets[i].active = 0;
                enemies[j].active = 0;
                *score += 10;
            }
        }
    }

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active)
            continue;
        SDL_Rect en = {(int)enemies[i].x, (int)enemies[i].y, 64, 64};
        if (SDL_HasIntersection(plane, &en)) {
            enemies[i].active = 0;
            playSound(sounds->hit);
            (*lives)--;
            if (*lives <= 0) {
                *finalScore = *score;
                *state = STATE_GAME_OVER;
            }
        }
    }

    SDL_RenderCopy(renderer, tex->plane1, NULL, plane);

    SDL_Rect r = {0, 0, 16, 16};
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active)
            continue;
        r.x = bullets[i].x;
        r.y = bullets[i].y;
        SDL_RenderCopyEx(renderer, tex->bullet, NULL, &r, -90, NULL, SDL_FLIP_NONE);
    }

    SDL_Rect er = {0, 0, 64, 64};
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active)
            continue;
        er.x = enemies[i].x;
        er.y = enemies[i].y;
        SDL_RenderCopyEx(renderer, tex->enemy, NULL, &er, 0, NULL, SDL_FLIP_VERTICAL);
    }

    SDL_Rect hr = {10, 10, 32, 32};
    for (int i = 0; i < *lives; i++) {
        hr.x = 10 + i * 40;
        SDL_RenderCopy(renderer, tex->heart, NULL, &hr);
    }

    char scoreText[64];
    sprintf(scoreText, "Score: %d", *score);
    renderTextTopRight(renderer, font, scoreText, (SDL_Color){0, 0, 0, 255});
}

void runClientMode(
    SDL_Renderer *renderer, TTF_Font *font,
    const GameTextures *tex,
    GameState *state, PlayMode *mode)
{
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    networkClientSetInput(
        keys[SDL_SCANCODE_LEFT],
        keys[SDL_SCANCODE_RIGHT],
        keys[SDL_SCANCODE_UP],
        keys[SDL_SCANCODE_DOWN]);

    NetGameState netState;
    if (!networkClientGetState(&netState)) {
        *state = STATE_MAIN_MENU;
        *mode  = MODE_LOCAL;
        return;
    }

    if (netState.gameOver) {
        networkClientDisconnect();
        *state = STATE_MAIN_MENU;
        *mode  = MODE_LOCAL;
        return;
    }

    SDL_Texture *playerTextures[NET_MAX_PLAYERS] = {tex->plane1, tex->plane2};
    for (int i = 0; i < NET_MAX_PLAYERS; i++) {
        if (!netState.players[i].active)
            continue;
        SDL_Rect r = {(int)netState.players[i].x, (int)netState.players[i].y, 64, 64};
        SDL_RenderCopy(renderer, playerTextures[i], NULL, &r);
    }

    SDL_Rect r = {0, 0, 16, 16};
    for (int i = 0; i < NET_MAX_BULLETS; i++) {
        if (!netState.bullets[i].active)
            continue;
        r.x = (int)netState.bullets[i].x;
        r.y = (int)netState.bullets[i].y;
        SDL_RenderCopyEx(renderer, tex->bullet, NULL, &r, -90, NULL, SDL_FLIP_NONE);
    }

    SDL_Rect er = {0, 0, 64, 64};
    for (int i = 0; i < NET_MAX_ENEMIES; i++) {
        if (!netState.enemies[i].active)
            continue;
        er.x = (int)netState.enemies[i].x;
        er.y = (int)netState.enemies[i].y;
        SDL_RenderCopyEx(renderer, tex->enemy, NULL, &er, 0, NULL, SDL_FLIP_VERTICAL);
    }

    SDL_Rect hr = {10, 10, 32, 32};
    for (int i = 0; i < netState.lives; i++) {
        hr.x = 10 + i * 40;
        SDL_RenderCopy(renderer, tex->heart, NULL, &hr);
    }

    char status[96];
    sprintf(status, "P%d  Players: %d  Score: %d",
            netState.playerId + 1,
            netState.connectedPlayers,
            netState.score);
    renderTextTopRight(renderer, font, status, (SDL_Color){0, 0, 0, 255});
}

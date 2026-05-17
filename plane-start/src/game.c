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
    if (!surf)
    {
        printf("Failed to load %s: %s\n", path, IMG_GetError());
        return NULL;
    }
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);
    return tex;
}

void loadGameTextures(SDL_Renderer *renderer, GameTextures *tex)
{
    tex->map = loadTexture(renderer, "resources/map.png");
    tex->plane1 = loadTexture(renderer, "resources/plane1.png");
    tex->plane2 = loadTexture(renderer, "resources/plane2.png");
    tex->bullet = loadTexture(renderer, "resources/test-bullet.png");
    tex->enemy = loadTexture(renderer, "resources/plane5.png");
    tex->heart = loadTexture(renderer, "resources/heart.png");
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
    int texW, texH;
    SDL_QueryTexture(mapTex, NULL, NULL, &texW, &texH);

    float scaleX = (float)SCREEN_WIDTH / texW;
    float scaleY = (float)SCREEN_HEIGHT / texH;
    float scale = scaleX > scaleY ? scaleX : scaleY;

    int scaledW = (int)(texW * scale);
    int scaledH = (int)(texH * scale);

    int x = (SCREEN_WIDTH - scaledW) / 2;
    int scrollY = (int)mapY % scaledH;

    SDL_Rect r1 = {x, scrollY, scaledW, scaledH};
    SDL_Rect r2 = {x, scrollY - scaledH, scaledW, scaledH};
    SDL_Rect r3 = {x, scrollY + scaledH, scaledW, scaledH};

    SDL_RenderCopy(renderer, mapTex, NULL, &r1);
    SDL_RenderCopy(renderer, mapTex, NULL, &r2);
    SDL_RenderCopy(renderer, mapTex, NULL, &r3);
}

void handleKeyDown(SDL_Event *event, GameState *state)
{
    if (event->key.repeat)
        return;

    if (event->key.keysym.sym == SDLK_ESCAPE)
    {
        if (*state == STATE_PLAYING)
            *state = STATE_PAUSE_MENU;
        else if (*state == STATE_PAUSE_MENU)
            *state = STATE_PLAYING;
    }
}

static const Uint8 PU_COL[PU_TYPE_COUNT][3] = {
    {255, 140, 0},
    {50, 160, 255},
    {50, 220, 80},
    {220, 50, 50},
};

static const char *PU_LABEL[PU_TYPE_COUNT] = {"R", "S", "T", "B"};
static const int PU_DURATION[PU_TYPE_COUNT] = {360, 300, 420, 0};

static void drawLabelCentered(SDL_Renderer *r, TTF_Font *font, const char *text,
                              SDL_Color col, int bx, int by, int bw, int bh)
{
    SDL_Surface *surf = TTF_RenderText_Blended(font, text, col);
    if (!surf)
        return;

    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
    SDL_Rect dst = {
        bx + (bw - surf->w) / 2,
        by + (bh - surf->h) / 2,
        surf->w,
        surf->h
    };

    SDL_FreeSurface(surf);
    SDL_RenderCopy(r, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
}

static void dropPowerUp(PowerUp powerUps[], float x, float y)
{
    for (int k = 0; k < MAX_POWERUPS; k++)
    {
        if (!powerUps[k].active)
        {
            powerUps[k] = (PowerUp){x, y, 1, rand() % PU_TYPE_COUNT};
            return;
        }
    }
}

static void applyPowerUp(PowerUpType type, PlayerEffects *effects,
                         Enemy enemies[], EnemyBullet enemyBullets[], int *score)
{
    switch (type)
    {
    case PU_RAPIDFIRE:
        effects->rapid = PU_DURATION[PU_RAPIDFIRE];
        break;

    case PU_SHIELD:
        effects->shield = PU_DURATION[PU_SHIELD];
        break;

    case PU_TRIPLE:
        effects->triple = PU_DURATION[PU_TRIPLE];
        break;

    case PU_BOMB:
        effects->bombFlash = 12;

        for (int i = 0; i < MAX_ENEMIES; i++)
        {
            if (enemies[i].active)
            {
                enemies[i].active = 0;
                *score += (enemies[i].type == 1) ? 30 : 10;
            }
        }

        for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
            enemyBullets[i].active = 0;

        break;

    default:
        break;
    }
}

void resetLocalGame(Player *player, Bullets *bullets, Enemy enemies[],
                    EnemyBullet enemyBullets[], PowerUp powerUps[],
                    PlayerEffects *effects, int *lives, int *score, int *shootTimer)
{
    playerReset(player);
    bulletsReset(bullets);

    memset(enemies, 0, sizeof(Enemy) * MAX_ENEMIES);
    memset(enemyBullets, 0, sizeof(EnemyBullet) * MAX_ENEMY_BULLETS);
    memset(powerUps, 0, sizeof(PowerUp) * MAX_POWERUPS);
    memset(effects, 0, sizeof(PlayerEffects));

    *lives = PLAYER_LIVES;
    *score = 0;
    *shootTimer = 0;
}

void runLocalMode(
    SDL_Renderer *renderer, TTF_Font *font,
    const GameTextures *tex, const GameSounds *sounds,
    Player *player, Bullets *bullets, Enemy enemies[],
    EnemyBullet enemyBullets[], PowerUp powerUps[], PlayerEffects *effects,
    int *shootTimer, int shootDelay,
    int *lives, int *score, int *finalScore,
    GameState *state)
{
    SDL_Rect *plane = playerGetMutableRect(player);

    if (effects->shield > 0)
        effects->shield--;
    if (effects->rapid > 0)
        effects->rapid--;
    if (effects->triple > 0)
        effects->triple--;
    if (effects->bombFlash > 0)
        effects->bombFlash--;

    const Uint8 *keys = SDL_GetKeyboardState(NULL);

    if (keys[SDL_SCANCODE_LEFT])
        plane->x -= 10;
    if (keys[SDL_SCANCODE_RIGHT])
        plane->x += 10;
    if (keys[SDL_SCANCODE_UP])
        plane->y -= 10;
    if (keys[SDL_SCANCODE_DOWN])
        plane->y += 10;

    if (plane->x < 0)
        plane->x = 0;
    if (plane->y < 0)
        plane->y = 0;
    if (plane->x > SCREEN_WIDTH - plane->w)
        plane->x = SCREEN_WIDTH - plane->w;
    if (plane->y > SCREEN_HEIGHT - plane->h)
        plane->y = SCREEN_HEIGHT - plane->h;

    int effectiveDelay = (effects->rapid > 0) ? 4 : shootDelay;

    (*shootTimer)++;

    if (*shootTimer >= effectiveDelay)
    {
        *shootTimer = 0;
        playSound(sounds->shoot);

#define PLAYER_BULLET_W 36
#define PLAYER_BULLET_H 14

        int gunCount = (effects->triple > 0) ? 3 : 2;
        float gunX[3];

        if (gunCount == 2)
        {
            gunX[0] = plane->x + plane->w * 0.25f - PLAYER_BULLET_W / 2;
            gunX[1] = plane->x + plane->w * 0.75f - PLAYER_BULLET_W / 2;
        }
        else
        {
            gunX[0] = plane->x + plane->w * 0.12f - PLAYER_BULLET_W / 2;
            gunX[1] = plane->x + plane->w * 0.50f - PLAYER_BULLET_W / 2;
            gunX[2] = plane->x + plane->w * 0.88f - PLAYER_BULLET_W / 2;
        }

        for (int g = 0; g < gunCount; g++)
        {
            bulletsAdd(bullets, gunX[g], plane->y);
        }
    }

    bulletsUpdate(bullets);

    int spawnChance = 5 + *score / 100;
    if (spawnChance > 25)
        spawnChance = 25;

    float baseSpeed = 2.0f + *score / 250.0f;
    if (baseSpeed > 5.0f)
        baseSpeed = 5.0f;

    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (!enemies[i].active && rand() % 1000 < spawnChance)
        {
            int tough = (rand() % 5 == 0);

            enemies[i] = (Enemy){
                .baseX = rand() % (SCREEN_WIDTH - 64),
                .y = -64,
                .offset = (float)(rand() % 360) / 50.0f,
                .active = 1,
                .type = tough,
                .health = tough ? 3 : 1,
                .shootTimer = 40 + rand() % 60
            };

            enemies[i].x = enemies[i].baseX;
        }

        if (!enemies[i].active)
            continue;

        float spd = (enemies[i].type == 1) ? baseSpeed * 0.7f : baseSpeed;

        enemies[i].y += spd;
        enemies[i].offset += 0.05f;
        enemies[i].x = enemies[i].baseX + 50 * sin(enemies[i].offset);

        if (enemies[i].y > SCREEN_HEIGHT)
        {
            enemies[i].active = 0;
            continue;
        }

        if (--enemies[i].shootTimer <= 0)
        {
            enemies[i].shootTimer = 40 + rand() % 60;

            for (int k = 0; k < MAX_ENEMY_BULLETS; k++)
            {
                if (!enemyBullets[k].active)
                {
                    enemyBullets[k] = (EnemyBullet){
                        enemies[i].x + 24,
                        enemies[i].y + 64,
                        1
                    };
                    break;
                }
            }
        }
    }

    for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
    {
        if (!enemyBullets[i].active)
            continue;

        enemyBullets[i].y += 6;

        if (enemyBullets[i].y > SCREEN_HEIGHT)
            enemyBullets[i].active = 0;
    }

    for (int i = 0; i < MAX_POWERUPS; i++)
    {
        if (!powerUps[i].active)
            continue;

        powerUps[i].y += 1.5f;

        if (powerUps[i].y > SCREEN_HEIGHT)
        {
            powerUps[i].active = 0;
            continue;
        }

        SDL_Rect pr = {
            (int)powerUps[i].x,
            (int)powerUps[i].y,
            32,
            32
        };

        if (SDL_HasIntersection(plane, &pr))
        {
            applyPowerUp(powerUps[i].type, effects, enemies, enemyBullets, score);
            powerUps[i].active = 0;
        }
    }

    for (int i = 0; i < MAX_BULLETS; i++)
    {
        for (int j = 0; j < MAX_ENEMIES; j++)
        {
            if (!bulletsIsActive(bullets, i) || !enemies[j].active)
                continue;

            SDL_Rect br = bulletsGetRect(bullets, i);
            SDL_Rect en = {
                (int)enemies[j].x,
                (int)enemies[j].y,
                64,
                64
            };

            if (!SDL_HasIntersection(&br, &en))
                continue;

            bulletsDeactivate(bullets, i);

            if (--enemies[j].health <= 0)
            {
                int dropChance = (enemies[j].type == 1) ? 40 : 20;

                if (rand() % 100 < dropChance)
                    dropPowerUp(powerUps, enemies[j].x + 16, enemies[j].y + 16);

                enemies[j].active = 0;
                *score += (enemies[j].type == 1) ? 30 : 10;
            }
        }
    }

    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (!enemies[i].active)
            continue;

        SDL_Rect en = {
            (int)enemies[i].x,
            (int)enemies[i].y,
            64,
            64
        };

        if (!SDL_HasIntersection(plane, &en))
            continue;

        enemies[i].active = 0;

        if (effects->shield > 0)
            continue;

        playSound(sounds->hit);

        if (--(*lives) <= 0)
        {
            *finalScore = *score;
            *state = STATE_GAME_OVER;
        }
    }

    for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
    {
        if (!enemyBullets[i].active)
            continue;

        SDL_Rect br = {
            (int)enemyBullets[i].x,
            (int)enemyBullets[i].y,
            16,
            16
        };

        if (!SDL_HasIntersection(plane, &br))
            continue;

        enemyBullets[i].active = 0;

        if (effects->shield > 0)
            continue;

        playSound(sounds->hit);

        if (--(*lives) <= 0)
        {
            *finalScore = *score;
            *state = STATE_GAME_OVER;
        }
    }

    if (effects->shield > 0 &&
        (effects->shield > 60 || (effects->shield / 8) % 2 == 0))
    {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 50, 160, 255, 160);

        SDL_Rect s1 = {
            plane->x - 5,
            plane->y - 5,
            plane->w + 10,
            plane->h + 10
        };

        SDL_Rect s2 = {
            plane->x - 8,
            plane->y - 8,
            plane->w + 16,
            plane->h + 16
        };

        SDL_RenderDrawRect(renderer, &s1);
        SDL_SetRenderDrawColor(renderer, 50, 160, 255, 80);
        SDL_RenderDrawRect(renderer, &s2);
    }

    SDL_RenderCopy(renderer, tex->plane1, NULL, plane);

    bulletsRender(bullets, renderer, tex->bullet);

    SDL_Rect r = {0, 0, 36, 14};

    SDL_SetTextureColorMod(tex->bullet, 255, 60, 60);

    for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
    {
        if (!enemyBullets[i].active)
            continue;

        r.x = (int)enemyBullets[i].x;
        r.y = (int)enemyBullets[i].y;

        SDL_RenderCopyEx(renderer, tex->bullet, NULL, &r, 90, NULL, SDL_FLIP_NONE);
    }

    SDL_SetTextureColorMod(tex->bullet, 255, 255, 255);

    SDL_Rect er = {0, 0, 64, 64};

    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (!enemies[i].active)
            continue;

        er.x = (int)enemies[i].x;
        er.y = (int)enemies[i].y;

        if (enemies[i].type == 1)
            SDL_SetTextureColorMod(tex->enemy, 255, 80, 80);

        SDL_RenderCopyEx(renderer, tex->enemy, NULL, &er, 0, NULL, SDL_FLIP_VERTICAL);
        SDL_SetTextureColorMod(tex->enemy, 255, 255, 255);
    }

    for (int i = 0; i < MAX_POWERUPS; i++)
    {
        if (!powerUps[i].active)
            continue;

        int px = (int)powerUps[i].x;
        int py = (int)powerUps[i].y;
        PowerUpType t = powerUps[i].type;

        float pulse = 0.6f + 0.4f * sinf((float)SDL_GetTicks() / 200.0f);
        Uint8 alpha = (Uint8)(200 * pulse);

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, PU_COL[t][0], PU_COL[t][1], PU_COL[t][2], alpha);

        SDL_Rect box = {px, py, 32, 32};

        SDL_RenderFillRect(renderer, &box);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 220);
        SDL_RenderDrawRect(renderer, &box);

        drawLabelCentered(
            renderer,
            font,
            PU_LABEL[t],
            (SDL_Color){255, 255, 255, 255},
            px,
            py,
            32,
            32
        );
    }

    if (effects->bombFlash > 0)
    {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        Uint8 a = (Uint8)(effects->bombFlash * 18);

        SDL_SetRenderDrawColor(renderer, 255, 220, 80, a);

        SDL_Rect full = {
            0,
            0,
            SCREEN_WIDTH,
            SCREEN_HEIGHT
        };

        SDL_RenderFillRect(renderer, &full);
    }

    SDL_Rect hr = {10, 10, 32, 32};

    for (int i = 0; i < *lives; i++)
    {
        hr.x = 10 + i * 40;
        SDL_RenderCopy(renderer, tex->heart, NULL, &hr);
    }

    typedef struct
    {
        int *timer;
        int maxTime;
        Uint8 r;
        Uint8 g;
        Uint8 b;
        const char *lbl;
    } Ind;

    Ind inds[3] = {
        {&effects->rapid, 360, 255, 140, 0, "R"},
        {&effects->triple, 420, 50, 220, 80, "T"},
        {&effects->shield, 300, 50, 160, 255, "S"},
    };

    int hudX = 10;

    for (int i = 0; i < 3; i++)
    {
        if (*inds[i].timer <= 0)
            continue;

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, inds[i].r, inds[i].g, inds[i].b, 190);

        SDL_Rect icon = {
            hudX,
            SCREEN_HEIGHT - 52,
            26,
            26
        };

        SDL_RenderFillRect(renderer, &icon);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
        SDL_RenderDrawRect(renderer, &icon);

        drawLabelCentered(
            renderer,
            font,
            inds[i].lbl,
            (SDL_Color){255, 255, 255, 255},
            hudX,
            SCREEN_HEIGHT - 52,
            26,
            26
        );

        int barFill = (int)(26 * (*inds[i].timer) / (float)inds[i].maxTime);

        SDL_SetRenderDrawColor(renderer, 40, 40, 40, 180);

        SDL_Rect bg = {
            hudX,
            SCREEN_HEIGHT - 22,
            26,
            5
        };

        SDL_RenderFillRect(renderer, &bg);

        SDL_SetRenderDrawColor(renderer, inds[i].r, inds[i].g, inds[i].b, 255);

        SDL_Rect fg = {
            hudX,
            SCREEN_HEIGHT - 22,
            barFill,
            5
        };

        SDL_RenderFillRect(renderer, &fg);

        hudX += 34;
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
        keys[SDL_SCANCODE_DOWN]
    );

    NetGameState netState;

    if (!networkClientGetState(&netState))
    {
        *state = STATE_MAIN_MENU;
        *mode = MODE_LOCAL;
        return;
    }

    if (netState.gameOver)
    {
        networkClientDisconnect();
        *state = STATE_MAIN_MENU;
        *mode = MODE_LOCAL;
        return;
    }

    SDL_Texture *playerTextures[NET_MAX_PLAYERS] = {
        tex->plane1,
        tex->plane2
    };

    for (int i = 0; i < NET_MAX_PLAYERS; i++)
    {
        if (!netState.players[i].active)
            continue;

        SDL_Rect r = {
            (int)netState.players[i].x,
            (int)netState.players[i].y,
            64,
            64
        };

        SDL_RenderCopy(renderer, playerTextures[i], NULL, &r);
    }

    SDL_Rect r = {0, 0, 16, 16};

    for (int i = 0; i < NET_MAX_BULLETS; i++)
    {
        if (!netState.bullets[i].active)
            continue;

        r.x = (int)netState.bullets[i].x;
        r.y = (int)netState.bullets[i].y;

        SDL_RenderCopyEx(renderer, tex->bullet, NULL, &r, -90, NULL, SDL_FLIP_NONE);
    }

    SDL_Rect er = {0, 0, 64, 64};

    for (int i = 0; i < NET_MAX_ENEMIES; i++)
    {
        if (!netState.enemies[i].active)
            continue;

        er.x = (int)netState.enemies[i].x;
        er.y = (int)netState.enemies[i].y;

        SDL_RenderCopyEx(renderer, tex->enemy, NULL, &er, 0, NULL, SDL_FLIP_VERTICAL);
    }

    SDL_Rect hr = {10, 10, 32, 32};

    for (int i = 0; i < netState.lives; i++)
    {
        hr.x = 10 + i * 40;
        SDL_RenderCopy(renderer, tex->heart, NULL, &hr);
    }

    char status[96];

    sprintf(
        status,
        "P%d  Players: %d  Score: %d",
        netState.playerId + 1,
        netState.connectedPlayers,
        netState.score
    );

    renderTextTopRight(renderer, font, status, (SDL_Color){0, 0, 0, 255});
}

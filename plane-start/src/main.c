#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include "network.h"
#include "UI.h"
#include "sound.h"

#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 800

#define MAX_BULLETS 100
#define MAX_ENEMIES 20
#define PLAYER_LIVES 4

// ---------------- GAME STATE ----------------
typedef enum
{
    STATE_MAIN_MENU,
    STATE_PLAYING,
    STATE_PAUSE_MENU,
    STATE_GAME_OVER,
    STATE_QUIT
} GameState;

typedef enum
{
    MODE_LOCAL,
    MODE_CLIENT
} PlayMode;

// ---------------- GAME OBJECTS ----------------
typedef struct
{
    float x, y;
    bool active;
} Bullet;

typedef struct
{
    float x, y;
    float baseX;
    float offset;
    int active;
} Enemy;

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

// ---------------- HELPERS ----------------
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

void renderScrollingBackground(SDL_Renderer *renderer, SDL_Texture *mapTex, float mapY)
{
    SDL_Rect mapRect1 = {
        0,
        (int)mapY,
        SCREEN_WIDTH,
        SCREEN_HEIGHT};

    SDL_Rect mapRect2 = {
        0,
        (int)mapY - SCREEN_HEIGHT,
        SCREEN_WIDTH,
        SCREEN_HEIGHT};

    SDL_RenderCopy(renderer, mapTex, NULL, &mapRect1);
    SDL_RenderCopy(renderer, mapTex, NULL, &mapRect2);
}

void drawCenterText(SDL_Renderer *renderer, TTF_Font *font, const char *text, int y, SDL_Color color)
{
    SDL_Surface *surface = TTF_RenderText_Blended(font, text, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

    int w, h;
    SDL_QueryTexture(texture, NULL, NULL, &w, &h);

    SDL_Rect rect = {
        SCREEN_WIDTH / 2 - w / 2,
        y,
        w,
        h};

    SDL_RenderCopy(renderer, texture, NULL, &rect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void renderGameOverMenu(SDL_Renderer *renderer, TTF_Font *font, SDL_Texture *buttonTex, int finalScore)
{
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color cyan = {0, 230, 255, 255};
    SDL_Color red = {255, 70, 70, 255};

    // SDL_SetRenderDrawColor(renderer, 55, 55, 150, 255);
    // SDL_RenderClear(renderer);

    SDL_Rect panel = {
        SCREEN_WIDTH / 2 - 360,
        110,
        720,
        590};

    SDL_SetRenderDrawColor(renderer, 20, 25, 60, 255);
    SDL_RenderFillRect(renderer, &panel);

    SDL_SetRenderDrawColor(renderer, 0, 220, 255, 255);
    SDL_RenderDrawRect(renderer, &panel);

    drawCenterText(renderer, font, "GAME OVER", 170, red);

    char scoreText[128];
    sprintf(scoreText, "YOUR SCORE: %d", finalScore);
    drawCenterText(renderer, font, scoreText, 250, cyan);

    SDL_Rect playAgainRect = {
        SCREEN_WIDTH / 2 - 230,
        380,
        460,
        95};

    SDL_Rect quitRect = {
        SCREEN_WIDTH / 2 - 230,
        510,
        460,
        95};

    SDL_RenderCopy(renderer, buttonTex, NULL, &playAgainRect);
    SDL_RenderCopy(renderer, buttonTex, NULL, &quitRect);

    drawCenterText(renderer, font, "PLAY AGAIN", 410, white);
    drawCenterText(renderer, font, "QUIT", 540, white);
}

// ---------------- STATE INPUT ----------------
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

// ---------------- BUTTON HANDLING ----------------
void handleButtonEvents(Button *b, SDL_Event *event, GameState *state, PlayMode *mode, const char *serverHost)
{
    int mx, my;
    SDL_GetMouseState(&mx, &my);

    b->isHovered = SDL_PointInRect(&(SDL_Point){mx, my}, &b->rect);

    if (event->type == SDL_MOUSEBUTTONDOWN &&
        event->button.button == SDL_BUTTON_LEFT &&
        b->isHovered)
    {
        if (*state == STATE_MAIN_MENU)
        {
            if (strcmp(b->label, "Start") == 0)
                *state = STATE_PLAYING;

            if (strcmp(b->label, "Quit") == 0)
                *state = STATE_QUIT;
            if (strcmp(b->label, "Client") == 0)
            {
                if (networkClientConnect(serverHost) == 0)
                {
                    *mode = MODE_CLIENT;
                    *state = STATE_PLAYING;
                }
            }
            if (strcmp(b->label, "Server") == 0)
                startServerThread();
        }
        else if (*state == STATE_PAUSE_MENU)
        {
            if (strcmp(b->label, "Resume") == 0)
                *state = STATE_PLAYING;

            if (strcmp(b->label, "Quit") == 0)
                *state = STATE_QUIT;
        }
    }
}

// ---------------- MAIN ----------------
int main(int argc, char *argv[])
{
    srand(time(NULL));
    const char *serverHost = argc > 1 ? argv[1] : SERVER_IP;

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();
    initAudio();

    SDL_Window *window = SDL_CreateWindow(
        "Shooter",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        0);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font *font = TTF_OpenFont("resources/fonts/DejaVuSans.ttf", 24);

    SDL_Texture *mapTex = loadTexture(renderer, "resources/map.png");
    SDL_Texture *planeTex = loadTexture(renderer, "resources/plane1.png");
    SDL_Texture *planeTex2 = loadTexture(renderer, "resources/plane2.png");
    SDL_Texture *bulletTex = loadTexture(renderer, "resources/test-bullet.png");
    SDL_Texture *enemyTex = loadTexture(renderer, "resources/plane5.png");
    SDL_Texture *heartTex = loadTexture(renderer, "resources/heart.png");

    Mix_Chunk *shootSound = loadSound("resources/sounds/bullet.wav");
    Mix_Chunk *hitSound = loadSound("resources/sounds/roblox.wav");

    Mix_Music *backgroundMusic = Mix_LoadMUS("resources/sounds/music.wav");

    if (!backgroundMusic)
    {
        printf("Failed to load music.wav: %s\n", Mix_GetError());
    }
    else
    {
        Mix_VolumeMusic(40);
        Mix_PlayMusic(backgroundMusic, -1);
    }

    SDL_Texture *buttonTex = loadTexture(renderer, "resources/buttonPictures/Button.png");

    SDL_SetTextureBlendMode(buttonTex, SDL_BLENDMODE_BLEND);
    SDL_Color text = {255, 255, 255, 255};

    float scale = 1.5;
    Button startButton = initializeButton(200 * scale, 60 * scale, buttonTex, text, "Start");
    Button quitButton = initializeButton(200 * scale, 60 * scale, buttonTex, text, "Quit");
    Button resumeButton = initializeButton(200 * scale, 60 * scale, buttonTex, text, "Resume");
    Button quitButton2 = initializeButton(200 * scale, 60 * scale, buttonTex, text, "Quit");
    Button serverButton = initializeButton(200 * scale, 60 * scale, buttonTex, text, "Server");
    Button clientButton = initializeButton(200 * scale, 60 * scale, buttonTex, text, "Client");

    Button mainMenuButtons[] = {startButton, quitButton, serverButton, clientButton};
    Button pauseMenuButtons[] = {resumeButton, quitButton2};

    GameState state = STATE_MAIN_MENU;
    PlayMode mode = MODE_LOCAL;

    SDL_Rect plane = {SCREEN_WIDTH / 2 - 32, SCREEN_HEIGHT - 80, 64, 64};

    Bullet bullets[MAX_BULLETS] = {0};
    Enemy enemies[MAX_ENEMIES] = {0};

    int shootTimer = 0;
    int shootDelay = 10;
    int lives = PLAYER_LIVES;
    int score = 0;
    int finalScore = 0;

    float mapY = 0;
    float mapSpeed = 1.2f;

    SDL_Event e;
    bool running = true;

    while (running)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                running = false;

            if (e.type == SDL_KEYDOWN)
                handleKeyDown(&e, &state);

            if (state == STATE_MAIN_MENU)
            {
                for (int i = 0; i < 4; i++)
                    handleButtonEvents(&mainMenuButtons[i], &e, &state, &mode, serverHost);
            }
            else if (state == STATE_PAUSE_MENU)
            {
                for (int i = 0; i < 2; i++)
                    handleButtonEvents(&pauseMenuButtons[i], &e, &state, &mode, serverHost);
            }
            else if (state == STATE_GAME_OVER)
            {
                int mx, my;
                SDL_GetMouseState(&mx, &my);

                SDL_Rect playAgainRect = {
                    SCREEN_WIDTH / 2 - 230,
                    380,
                    460,
                    95};

                SDL_Rect quitRect = {
                    SCREEN_WIDTH / 2 - 230,
                    510,
                    460,
                    95};

                if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
                {
                    if (SDL_PointInRect(&(SDL_Point){mx, my}, &playAgainRect))
                    {
                        resetLocalGame(&plane, bullets, enemies, &lives, &score, &shootTimer);
                        state = STATE_PLAYING;
                    }

                    if (SDL_PointInRect(&(SDL_Point){mx, my}, &quitRect))
                    {
                        state = STATE_QUIT;
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        mapY += mapSpeed;

        if (mapY >= SCREEN_HEIGHT)
        {
            mapY = 0;
        }

        renderScrollingBackground(renderer, mapTex, mapY);

        if (state == STATE_MAIN_MENU)
        {
            renderMenu(renderer, mainMenuButtons, 4, font, 400, 600);
        }
        else if (state == STATE_PLAYING)
        {
            const Uint8 *keys = SDL_GetKeyboardState(NULL);

            if (mode == MODE_CLIENT)
            {
                NetGameState netState;
                networkClientSetInput(
                    keys[SDL_SCANCODE_LEFT],
                    keys[SDL_SCANCODE_RIGHT],
                    keys[SDL_SCANCODE_UP],
                    keys[SDL_SCANCODE_DOWN]);

                if (!networkClientGetState(&netState))
                {
                    state = STATE_MAIN_MENU;
                    mode = MODE_LOCAL;
                }
                else
                {
                    if (netState.gameOver)
                    {
                        networkClientDisconnect();
                        state = STATE_MAIN_MENU;
                        mode = MODE_LOCAL;
                    }

                    SDL_Texture *playerTextures[NET_MAX_PLAYERS] = {planeTex, planeTex2};
                    for (int i = 0; i < NET_MAX_PLAYERS; i++)
                    {
                        if (!netState.players[i].active)
                            continue;

                        SDL_Rect playerRect = {
                            (int)netState.players[i].x,
                            (int)netState.players[i].y,
                            64,
                            64};
                        SDL_RenderCopy(renderer, playerTextures[i], NULL, &playerRect);
                    }

                    SDL_Rect r = {0, 0, 16, 16};
                    for (int i = 0; i < NET_MAX_BULLETS; i++)
                    {
                        if (!netState.bullets[i].active)
                            continue;

                        r.x = (int)netState.bullets[i].x;
                        r.y = (int)netState.bullets[i].y;
                        SDL_RenderCopyEx(renderer, bulletTex, NULL, &r, -90, NULL, SDL_FLIP_NONE);
                    }

                    SDL_Rect er = {0, 0, 64, 64};
                    for (int i = 0; i < NET_MAX_ENEMIES; i++)
                    {
                        if (!netState.enemies[i].active)
                            continue;

                        er.x = (int)netState.enemies[i].x;
                        er.y = (int)netState.enemies[i].y;
                        SDL_RenderCopyEx(renderer, enemyTex, NULL, &er, 0, NULL, SDL_FLIP_VERTICAL);
                    }

                    SDL_Rect hr = {10, 10, 32, 32};
                    for (int i = 0; i < netState.lives; i++)
                    {
                        hr.x = 10 + i * 40;
                        SDL_RenderCopy(renderer, heartTex, NULL, &hr);
                    }

                    char statusText[96];
                    sprintf(statusText, "P%d  Players: %d  Score: %d",
                            netState.playerId + 1,
                            netState.connectedPlayers,
                            netState.score);

                    SDL_Surface *statusSurface = TTF_RenderText_Blended(font, statusText, (SDL_Color){0, 0, 0, 255});
                    SDL_Texture *statusTexture = SDL_CreateTextureFromSurface(renderer, statusSurface);
                    int tw, th;
                    SDL_QueryTexture(statusTexture, NULL, NULL, &tw, &th);
                    SDL_Rect statusRect = {SCREEN_WIDTH - tw - 20, 10, tw, th};
                    SDL_RenderCopy(renderer, statusTexture, NULL, &statusRect);
                    SDL_FreeSurface(statusSurface);
                    SDL_DestroyTexture(statusTexture);
                }
            }
            else
            {

                if (keys[SDL_SCANCODE_LEFT])
                    plane.x -= 5;
                if (keys[SDL_SCANCODE_RIGHT])
                    plane.x += 5;
                if (keys[SDL_SCANCODE_UP])
                    plane.y -= 5;
                if (keys[SDL_SCANCODE_DOWN])
                    plane.y += 5;

                if (plane.x < 0)
                    plane.x = 0;
                if (plane.y < 0)
                    plane.y = 0;
                if (plane.x > SCREEN_WIDTH - plane.w)
                    plane.x = SCREEN_WIDTH - plane.w;
                if (plane.y > SCREEN_HEIGHT - plane.h)
                    plane.y = SCREEN_HEIGHT - plane.h;

                shootTimer++;

                int leftGun = plane.x + plane.w / 4 - 8;
                int rightGun = plane.x + 3 * plane.w / 4 - 8;

                if (shootTimer >= shootDelay)
                {
                    shootTimer = 0;
                    playSound(shootSound);

                    for (int i = 0; i < MAX_BULLETS; i++)
                    {
                        if (!bullets[i].active)
                        {
                            bullets[i].x = leftGun;
                            bullets[i].y = plane.y;
                            bullets[i].active = 1;
                            break;
                        }
                    }

                    for (int i = 0; i < MAX_BULLETS; i++)
                    {
                        if (!bullets[i].active)
                        {
                            bullets[i].x = rightGun;
                            bullets[i].y = plane.y;
                            bullets[i].active = 1;
                            break;
                        }
                    }
                }

                for (int i = 0; i < MAX_BULLETS; i++)
                    if (bullets[i].active)
                    {
                        bullets[i].y -= 8;
                        if (bullets[i].y < 0)
                            bullets[i].active = 0;
                    }

                for (int i = 0; i < MAX_ENEMIES; i++)
                {
                    if (!enemies[i].active && rand() % 1000 < 5)
                    {
                        enemies[i].baseX = rand() % (SCREEN_WIDTH - 64);
                        enemies[i].x = enemies[i].baseX;
                        enemies[i].y = -64;
                        enemies[i].offset = (float)(rand() % 360) / 50.0f;
                        enemies[i].active = 1;
                    }

                    if (enemies[i].active)
                    {
                        enemies[i].y += 2;
                        enemies[i].offset += 0.05f;
                        enemies[i].x = enemies[i].baseX + 50 * sin(enemies[i].offset);

                        if (enemies[i].y > SCREEN_HEIGHT)
                            enemies[i].active = 0;
                    }
                }

                for (int i = 0; i < MAX_BULLETS; i++)
                    for (int j = 0; j < MAX_ENEMIES; j++)
                    {
                        if (!bullets[i].active || !enemies[j].active)
                            continue;

                        SDL_Rect b = {(int)bullets[i].x, (int)bullets[i].y, 16, 16};
                        SDL_Rect en = {(int)enemies[j].x, (int)enemies[j].y, 64, 64};

                        if (SDL_HasIntersection(&b, &en))
                        {
                            bullets[i].active = 0;
                            enemies[j].active = 0;
                            score += 10;
                        }
                    }

                for (int i = 0; i < MAX_ENEMIES; i++)
                {
                    if (!enemies[i].active)
                        continue;

                    SDL_Rect en = {(int)enemies[i].x, (int)enemies[i].y, 64, 64};

                    if (SDL_HasIntersection(&plane, &en))
                    {
                        enemies[i].active = 0;
                        playSound(hitSound);
                        lives--;

                        if (lives <= 0)
                        {
                            finalScore = score;
                            state = STATE_GAME_OVER;
                        }
                    }
                }

                SDL_RenderCopy(renderer, planeTex, NULL, &plane);

                SDL_Rect r = {0, 0, 16, 16};
                for (int i = 0; i < MAX_BULLETS; i++)
                    if (bullets[i].active)
                    {
                        r.x = bullets[i].x;
                        r.y = bullets[i].y;
                        SDL_RenderCopyEx(renderer, bulletTex, NULL, &r, -90, NULL, SDL_FLIP_NONE);
                    }

                SDL_Rect er = {0, 0, 64, 64};
                for (int i = 0; i < MAX_ENEMIES; i++)
                    if (enemies[i].active)
                    {
                        er.x = enemies[i].x;
                        er.y = enemies[i].y;
                        SDL_RenderCopyEx(renderer, enemyTex, NULL, &er, 0, NULL, SDL_FLIP_VERTICAL);
                    }

                SDL_Rect hr = {10, 10, 32, 32};
                for (int i = 0; i < lives; i++)
                {
                    hr.x = 10 + i * 40;
                    SDL_RenderCopy(renderer, heartTex, NULL, &hr);
                }

                char scoreText[64];
                sprintf(scoreText, "Score: %d", score);

                SDL_Surface *scoreSurface = TTF_RenderText_Blended(font, scoreText, (SDL_Color){0, 0, 0, 255});
                SDL_Texture *scoreTexture = SDL_CreateTextureFromSurface(renderer, scoreSurface);

                int tw, th;
                SDL_QueryTexture(scoreTexture, NULL, NULL, &tw, &th);

                SDL_Rect scoreRect = {SCREEN_WIDTH - tw - 20, 10, tw, th};

                SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);

                SDL_FreeSurface(scoreSurface);
                SDL_DestroyTexture(scoreTexture);
            }
        }
        else if (state == STATE_GAME_OVER)
        {
            renderGameOverMenu(renderer, font, buttonTex, finalScore);
        }
        else if (state == STATE_PAUSE_MENU)
        {
            renderMenu(renderer, pauseMenuButtons, 2, font, 400, 600);
        }
        else if (state == STATE_QUIT)
        {
            running = false;
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

// Rensar alla laddade resurser och stänger ner SDL-systemen när spelet avslutas
    SDL_DestroyTexture(mapTex);
    SDL_DestroyTexture(planeTex);
    SDL_DestroyTexture(planeTex2);
    SDL_DestroyTexture(bulletTex);
    SDL_DestroyTexture(enemyTex);
    SDL_DestroyTexture(heartTex);
    SDL_DestroyTexture(buttonTex);

    Mix_FreeChunk(shootSound);
    Mix_FreeChunk(hitSound);
    Mix_FreeMusic(backgroundMusic);

    Mix_CloseAudio();
    Mix_Quit();
    TTF_Quit();
    IMG_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

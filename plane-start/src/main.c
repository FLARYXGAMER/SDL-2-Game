#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "constants.h"
#include "game.h"
#include "network.h"
#include "sound.h"
#include "UI.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof(a[0]))

static void handleButtonEvents(Button *b, SDL_Event *event, GameState *state, PlayMode *mode, const char *serverHost)
{
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    b->isHovered = SDL_PointInRect(&(SDL_Point){mx, my}, &b->rect);

    if (event->type != SDL_MOUSEBUTTONDOWN ||
        event->button.button != SDL_BUTTON_LEFT ||
        !b->isHovered)
        return;

    if (*state == STATE_MAIN_MENU) {
        if (strcmp(b->label, "Start") == 0)
            *state = STATE_PLAYING;
        else if (strcmp(b->label, "Quit") == 0)
            *state = STATE_QUIT;
        else if (strcmp(b->label, "Client") == 0) {
            if (networkClientConnect(serverHost) == 0) {
                *mode  = MODE_CLIENT;
                *state = STATE_PLAYING;
            }
        } else if (strcmp(b->label, "Server") == 0)
            startServerThread();
    } else if (*state == STATE_PAUSE_MENU) {
        if (strcmp(b->label, "Resume") == 0)
            *state = STATE_PLAYING;
        else if (strcmp(b->label, "Quit") == 0)
            *state = STATE_QUIT;
    } else if (*state == STATE_GAME_OVER) {
        if (strcmp(b->label, "Play Again") == 0)
            *state = STATE_PLAYING;
        else if (strcmp(b->label, "Quit") == 0)
            *state = STATE_QUIT;
    }
}

int main(int argc, char *argv[])
{
    srand(time(NULL));
    const char *serverHost = argc > 1 ? argv[1] : SERVER_IP;

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();
    initAudio();

    SDL_Window   *window   = SDL_CreateWindow("Shooter", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font     *font     = TTF_OpenFont("resources/fonts/DejaVuSans.ttf", 24);

    SDL_Texture *mapTex    = loadTexture(renderer, "resources/map.png");
    SDL_Texture *planeTex  = loadTexture(renderer, "resources/plane1.png");
    SDL_Texture *planeTex2 = loadTexture(renderer, "resources/plane2.png");
    SDL_Texture *bulletTex = loadTexture(renderer, "resources/test-bullet.png");
    SDL_Texture *enemyTex  = loadTexture(renderer, "resources/plane5.png");
    SDL_Texture *heartTex  = loadTexture(renderer, "resources/heart.png");
    SDL_Texture *buttonTex = loadTexture(renderer, "resources/buttonPictures/Button.png");
    SDL_SetTextureBlendMode(buttonTex, SDL_BLENDMODE_BLEND);

    Mix_Chunk *shootSound = loadSound("resources/sounds/bullet.wav");
    Mix_Chunk *hitSound   = loadSound("resources/sounds/roblox.wav");

    Mix_Music *bgMusic = Mix_LoadMUS("resources/sounds/music.wav");
    if (!bgMusic)
        printf("Failed to load music.wav: %s\n", Mix_GetError());
    else {
        Mix_VolumeMusic(40);
        Mix_PlayMusic(bgMusic, -1);
    }

    SDL_Color white = {255, 255, 255, 255};
    float scale = 1.5f;

    Button mainMenuButtons[] = {
        initializeButton(200 * scale, 60 * scale, buttonTex, white, "Start"),
        initializeButton(200 * scale, 60 * scale, buttonTex, white, "Quit"),
        initializeButton(200 * scale, 60 * scale, buttonTex, white, "Server"),
        initializeButton(200 * scale, 60 * scale, buttonTex, white, "Client"),
    };
    Button pauseMenuButtons[] = {
        initializeButton(200 * scale, 60 * scale, buttonTex, white, "Resume"),
        initializeButton(200 * scale, 60 * scale, buttonTex, white, "Quit"),
    };
    Button gameOverButtons[] = {
        initializeButton(200 * scale, 60 * scale, buttonTex, white, "Play Again"),
        initializeButton(200 * scale, 60 * scale, buttonTex, white, "Quit"),
    };

    GameState state = STATE_MAIN_MENU;
    PlayMode  mode  = MODE_LOCAL;

    SDL_Rect plane = {SCREEN_WIDTH / 2 - 32, SCREEN_HEIGHT - 80, 64, 64};

    Bullet bullets[MAX_BULLETS] = {0};
    Enemy  enemies[MAX_ENEMIES] = {0};

    int shootTimer = 0;
    int shootDelay = 10;
    int lives      = PLAYER_LIVES;
    int score      = 0;
    int finalScore = 0;

    float mapY     = 0;
    float mapSpeed = 1.2f;

    SDL_Event e;
    bool running = true;

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                running = false;

            if (e.type == SDL_KEYDOWN)
                handleKeyDown(&e, &state);

            Button *buttons = NULL;
            int     count   = 0;
            if (state == STATE_MAIN_MENU) {
                buttons = mainMenuButtons;
                count   = ARRAY_LEN(mainMenuButtons);
            } else if (state == STATE_PAUSE_MENU) {
                buttons = pauseMenuButtons;
                count   = ARRAY_LEN(pauseMenuButtons);
            } else if (state == STATE_GAME_OVER) {
                buttons = gameOverButtons;
                count   = ARRAY_LEN(gameOverButtons);
            }

            GameState prevState = state;
            for (int i = 0; i < count; i++)
                handleButtonEvents(&buttons[i], &e, &state, &mode, serverHost);
            if (prevState == STATE_GAME_OVER && state == STATE_PLAYING)
                resetLocalGame(&plane, bullets, enemies, &lives, &score, &shootTimer);
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        mapY += mapSpeed;
        if (mapY >= SCREEN_HEIGHT)
            mapY = 0;
        renderScrollingBackground(renderer, mapTex, mapY);

        if (state == STATE_MAIN_MENU) {
            renderMenu(renderer, mainMenuButtons, ARRAY_LEN(mainMenuButtons), font, 400, 600);
        } else if (state == STATE_PLAYING) {
            if (mode == MODE_CLIENT)
                runClientMode(renderer, font, planeTex, planeTex2, bulletTex, enemyTex, heartTex, &state, &mode);
            else
                runLocalMode(renderer, font, planeTex, bulletTex, enemyTex, heartTex,
                             shootSound, hitSound, &plane, bullets, enemies,
                             &shootTimer, shootDelay, &lives, &score, &finalScore, &state);
        } else if (state == STATE_GAME_OVER) {
            renderGameOverMenu(renderer, font, gameOverButtons, ARRAY_LEN(gameOverButtons), finalScore);
        } else if (state == STATE_PAUSE_MENU) {
            renderMenu(renderer, pauseMenuButtons, ARRAY_LEN(pauseMenuButtons), font, 400, 600);
        } else if (state == STATE_QUIT) {
            running = false;
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyTexture(mapTex);
    SDL_DestroyTexture(planeTex);
    SDL_DestroyTexture(planeTex2);
    SDL_DestroyTexture(bulletTex);
    SDL_DestroyTexture(enemyTex);
    SDL_DestroyTexture(heartTex);
    SDL_DestroyTexture(buttonTex);

    Mix_FreeChunk(shootSound);
    Mix_FreeChunk(hitSound);
    Mix_FreeMusic(bgMusic);

    cleanupAudio();

    TTF_Quit();
    IMG_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

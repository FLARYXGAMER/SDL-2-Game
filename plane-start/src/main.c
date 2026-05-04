#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
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

static void initSDL(SDL_Window **window, SDL_Renderer **renderer, TTF_Font **font)
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();
    initAudio();
    *window   = SDL_CreateWindow("Shooter", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    *font     = TTF_OpenFont("resources/fonts/DejaVuSans.ttf", 24);
}

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

    SDL_Window   *window;
    SDL_Renderer *renderer;
    TTF_Font     *font;
    initSDL(&window, &renderer, &font);

    GameTextures tex;
    loadGameTextures(renderer, &tex);

    GameSounds sounds;
    loadGameSounds(&sounds);

    MenuButtons menus;
    initMenuButtons(&menus, tex.button);

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
                buttons = menus.mainMenu;
                count   = MAIN_MENU_COUNT;
            } else if (state == STATE_PAUSE_MENU) {
                buttons = menus.pauseMenu;
                count   = PAUSE_MENU_COUNT;
            } else if (state == STATE_GAME_OVER) {
                buttons = menus.gameOver;
                count   = GAME_OVER_COUNT;
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
        renderScrollingBackground(renderer, tex.map, mapY);

        if (state == STATE_MAIN_MENU) {
            renderMenu(renderer, menus.mainMenu, MAIN_MENU_COUNT, font, 400, 600);
        } else if (state == STATE_PLAYING) {
            if (mode == MODE_CLIENT)
                runClientMode(renderer, font, &tex, &state, &mode);
            else
                runLocalMode(renderer, font, &tex, &sounds, &plane, bullets, enemies,
                             &shootTimer, shootDelay, &lives, &score, &finalScore, &state);
        } else if (state == STATE_GAME_OVER) {
            renderGameOverMenu(renderer, font, menus.gameOver, GAME_OVER_COUNT, finalScore);
        } else if (state == STATE_PAUSE_MENU) {
            renderMenu(renderer, menus.pauseMenu, PAUSE_MENU_COUNT, font, 400, 600);
        } else if (state == STATE_QUIT) {
            running = false;
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    destroyGameTextures(&tex);
    freeGameSounds(&sounds);
    cleanupAudio();

    TTF_Quit();
    IMG_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

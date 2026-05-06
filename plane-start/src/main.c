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

    *window = SDL_CreateWindow(
        "Shooter",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_FULLSCREEN_DESKTOP);

    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);

    /*
        Viktigt:
        Spelet använder fortfarande SCREEN_WIDTH och SCREEN_HEIGHT internt.
        SDL skalar sedan upp spelet till fullscreen.
    */
    SDL_RenderSetLogicalSize(*renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

    *font = TTF_OpenFont("resources/fonts/DejaVuSans.ttf", 24);

    if (!*font)
    {
        printf("Failed to load font: %s\n", TTF_GetError());
    }
}

static void handleButtonEvents(SDL_Renderer *renderer, Button *b, SDL_Event *event,
                               GameState *state, PlayMode *mode, const char *serverHost)
{
    int mx, my;
    float logicalX, logicalY;

    SDL_GetMouseState(&mx, &my);

    /*
        Detta fixar fullscreen-problemet:
        Musens riktiga skärmposition görs om till spelets interna koordinater.
    */
    SDL_RenderWindowToLogical(renderer, mx, my, &logicalX, &logicalY);

    SDL_Point mousePoint = {
        (int)logicalX,
        (int)logicalY};

    b->isHovered = SDL_PointInRect(&mousePoint, &b->rect);

    if (event->type != SDL_MOUSEBUTTONDOWN ||
        event->button.button != SDL_BUTTON_LEFT ||
        !b->isHovered)
        return;

    if (*state == STATE_MAIN_MENU)
    {
        if (strcmp(b->label, "Start") == 0)
            *state = STATE_PLAYING;

        else if (strcmp(b->label, "Quit") == 0)
            *state = STATE_QUIT;

        else if (strcmp(b->label, "Client") == 0)
        {
            if (networkClientConnect(serverHost) == 0)
            {
                *mode = MODE_CLIENT;
                *state = STATE_PLAYING;
            }
        }

        else if (strcmp(b->label, "Server") == 0)
        {
            startServerThread();
        }
    }

    else if (*state == STATE_PAUSE_MENU)
    {
        if (strcmp(b->label, "Resume") == 0)
            *state = STATE_PLAYING;

        else if (strcmp(b->label, "Quit") == 0)
            *state = STATE_QUIT;
    }

    else if (*state == STATE_GAME_OVER)
    {
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

    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *font;

    initSDL(&window, &renderer, &font);

    GameTextures tex;
    loadGameTextures(renderer, &tex);

    GameSounds sounds;
    loadGameSounds(&sounds);

    MenuButtons menus;
    initMenuButtons(&menus, tex.button);

    GameState state = STATE_MAIN_MENU;
    PlayMode mode = MODE_LOCAL;

    /*
        Flygplanet är nu 96x96.
        Vill du göra det mindre/större ändrar du de två sista värdena.
    */
    SDL_Rect plane = {
        SCREEN_WIDTH / 2 - 48,
        SCREEN_HEIGHT - 116,
        96,
        96};

    Bullet bullets[MAX_BULLETS] = {0};
    Enemy enemies[MAX_ENEMIES] = {0};
    EnemyBullet enemyBullets[MAX_ENEMY_BULLETS] = {0};
    PowerUp powerUps[MAX_POWERUPS] = {0};
    PlayerEffects effects = {0};

    int shootTimer = 0;
    int shootDelay = 10;
    int lives = PLAYER_LIVES;
    int score = 0;
    int finalScore = 0;

    float mapY = 0;
    float mapSpeed = 1.2f;

    SDL_Event e;
    bool running = true;
    bool fullscreen = true;

    while (running)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                running = false;
            }

            if (e.type == SDL_KEYDOWN)
            {
                handleKeyDown(&e, &state);

                /*
                    Tryck F för att växla mellan fullscreen och fönsterläge.
                */
                if (e.key.keysym.sym == SDLK_f)
                {
                    fullscreen = !fullscreen;

                    if (fullscreen)
                    {
                        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                    }
                    else
                    {
                        SDL_SetWindowFullscreen(window, 0);
                    }
                }
            }

            Button *buttons = NULL;
            int count = 0;

            if (state == STATE_MAIN_MENU)
            {
                buttons = menus.mainMenu;
                count = MAIN_MENU_COUNT;
            }
            else if (state == STATE_PAUSE_MENU)
            {
                buttons = menus.pauseMenu;
                count = PAUSE_MENU_COUNT;
            }
            else if (state == STATE_GAME_OVER)
            {
                buttons = menus.gameOver;
                count = GAME_OVER_COUNT;
            }

            GameState prevState = state;

            for (int i = 0; i < count; i++)
            {
                handleButtonEvents(renderer, &buttons[i], &e, &state, &mode, serverHost);
            }

            if (prevState == STATE_GAME_OVER && state == STATE_PLAYING)
            {
                resetLocalGame(
                    &plane,
                    bullets,
                    enemies,
                    enemyBullets,
                    powerUps,
                    &effects,
                    &lives,
                    &score,
                    &shootTimer);
            }
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        mapY += mapSpeed;

        /*
            Nollställ inte vid SCREEN_HEIGHT längre.
            Bakgrunden sköter loopningen själv i renderScrollingBackground.
        */
        if (mapY > 100000.0f)
            mapY = 0;

        renderScrollingBackground(renderer, tex.map, mapY);

        if (state == STATE_MAIN_MENU)
        {
            renderMenu(
                renderer,
                menus.mainMenu,
                MAIN_MENU_COUNT,
                font,
                460,
                420,
                "AIRSTRIKE");
        }

        else if (state == STATE_PLAYING)
        {
            if (mode == MODE_CLIENT)
            {
                runClientMode(renderer, font, &tex, &state, &mode);
            }
            else
            {
                runLocalMode(
                    renderer,
                    font,
                    &tex,
                    &sounds,
                    &plane,
                    bullets,
                    enemies,
                    enemyBullets,
                    powerUps,
                    &effects,
                    &shootTimer,
                    shootDelay,
                    &lives,
                    &score,
                    &finalScore,
                    &state);
            }

            mapSpeed = 1.2f + score / 500.0f;

            if (mapSpeed > 3.0f)
                mapSpeed = 3.0f;
        }

        else if (state == STATE_GAME_OVER)
        {
            renderGameOverMenu(
                renderer,
                font,
                menus.gameOver,
                GAME_OVER_COUNT,
                finalScore);
        }

        else if (state == STATE_PAUSE_MENU)
        {
            renderMenu(
                renderer,
                menus.pauseMenu,
                PAUSE_MENU_COUNT,
                font,
                380,
                280,
                "PAUSED");
        }

        else if (state == STATE_QUIT)
        {
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
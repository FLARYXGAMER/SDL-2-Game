#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>
#include "network.h"
#include "UI.h"
#include "sound.h"

#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 800

#define MAX_BULLETS 100
#define MAX_ENEMIES 20
#define PLAYER_LIVES 4

// ---------------- GAME STATE ----------------
typedef enum {
    STATE_MAIN_MENU,
    STATE_PLAYING,
    STATE_PAUSE_MENU,
    STATE_QUIT
} GameState;

// ---------------- GAME OBJECTS ----------------
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

// ---------------- HELPERS ----------------
SDL_Texture* loadTexture(SDL_Renderer* renderer, const char* path)
{
    SDL_Surface* surf = IMG_Load(path);
    if (!surf) {
        printf("Failed to load %s: %s\n", path, IMG_GetError());
        return NULL;
    }

    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);
    return tex;
}

// ---------------- STATE INPUT ----------------
void handleKeyDown(SDL_Event* event, GameState* state)
{
    if (event->key.repeat) return;

    if (event->key.keysym.sym == SDLK_ESCAPE)
    {
        if (*state == STATE_PLAYING)
            *state = STATE_PAUSE_MENU;
        else if (*state == STATE_PAUSE_MENU)
            *state = STATE_PLAYING;
    }
}

// ---------------- BUTTON HANDLING ----------------
void handleButtonEvents(Button* b, SDL_Event* event, GameState* state)
{
    int mx, my;
    SDL_GetMouseState(&mx, &my);

    b->isHovered = SDL_PointInRect(&(SDL_Point){mx, my}, &b->rect);

    if (event->type == SDL_MOUSEBUTTONDOWN &&
        event->button.button == SDL_BUTTON_LEFT &&
        b->isHovered)
    {
        // -------- MAIN MENU --------
        if (*state == STATE_MAIN_MENU)
        {
            if (strcmp(b->label, "Start") == 0)
                *state = STATE_PLAYING;

            if (strcmp(b->label, "Quit") == 0)
                *state = STATE_QUIT;
            if (strcmp(b->label, "Client") == 0)
                testClientFun();
            if (strcmp(b->label, "Server") == 0) {
                pthread_t t;
                pthread_create(&t, NULL, serverThread, NULL);
                pthread_detach(t);
            }
        }

        // -------- PAUSE MENU --------
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
int main(int argc, char* argv[])
{
    srand(time(NULL));

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();
    initAudio();

    SDL_Window* window = SDL_CreateWindow(
        "Shooter",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        0
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font* font = TTF_OpenFont("resources/fonts/DejaVuSans.ttf", 24);

    // ---------------- TEXTURES ----------------
    SDL_Texture* planeTex  = loadTexture(renderer, "resources/plane1.png");
    SDL_Texture* bulletTex = loadTexture(renderer, "resources/test-bullet.png");
    SDL_Texture* enemyTex  = loadTexture(renderer, "resources/plane5.png");
    SDL_Texture* heartTex  = loadTexture(renderer, "resources/heart.png");

    // ---------------- SOUND ----------------
    Mix_Chunk* shootSound = loadSound("resources/sounds/bullet.wav");
    Mix_Chunk* hitSound   = loadSound("resources/sounds/roblox.wav");

    // ---------------- BUTTONS ----------------
    SDL_Texture* buttonTex = loadTexture(renderer, "resources/buttonPictures/Button.png");
    SDL_SetTextureBlendMode(buttonTex, SDL_BLENDMODE_BLEND);
    SDL_Color text = {255, 255, 255, 255};
    float scale = 1.5;
    Button startButton  = initializeButton(200*scale, 60 *scale, buttonTex, text, "Start");
    Button quitButton   = initializeButton(200*scale, 60 *scale, buttonTex, text, "Quit");
    Button resumeButton = initializeButton(200*scale, 60 *scale, buttonTex, text, "Resume");
    Button quitButton2  = initializeButton(200*scale, 60 *scale, buttonTex, text, "Quit");
    Button serverButton = initializeButton(200*scale, 60 *scale, buttonTex, text, "Server");
    Button clientButton = initializeButton(200*scale, 60 *scale, buttonTex, text, "Client");

    Button mainMenuButtons[]  = { startButton,  quitButton, serverButton, clientButton};
    Button pauseMenuButtons[] = { resumeButton, quitButton2 };

    // ---------------- GAME STATE ----------------
    GameState state = STATE_MAIN_MENU;

    // ---------------- PLAYER ----------------
    SDL_Rect plane = {SCREEN_WIDTH/2 - 32, SCREEN_HEIGHT - 80, 64, 64};

    Bullet bullets[MAX_BULLETS] = {0};
    Enemy enemies[MAX_ENEMIES] = {0};

    int shootTimer = 0;
    int shootDelay = 10;
    int lives = PLAYER_LIVES;
    int score = 0;

    SDL_Event e;
    bool running = true;

    // ---------------- MAIN LOOP ----------------
    while (running)
    {
        // ============ INPUT ============
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                running = false;

            if (e.type == SDL_KEYDOWN)
                handleKeyDown(&e, &state);

            if (state == STATE_MAIN_MENU)
            {
                for (int i = 0; i < 4; i++)
                    handleButtonEvents(&mainMenuButtons[i], &e, &state);
            }
            else if (state == STATE_PAUSE_MENU)
            {
                for (int i = 0; i < 2; i++)
                    handleButtonEvents(&pauseMenuButtons[i], &e, &state);
            }
        }

        // ============ RENDER CLEAR ============
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        // ================= MAIN MENU =================
        if (state == STATE_MAIN_MENU)
        {
            renderMenu(renderer, mainMenuButtons, 4, font, 400, 600);
        }

        // ================= PLAYING =================
        else if (state == STATE_PLAYING)
        {
            const Uint8* keys = SDL_GetKeyboardState(NULL);

            if (keys[SDL_SCANCODE_LEFT])  plane.x -= 5;
            if (keys[SDL_SCANCODE_RIGHT]) plane.x += 5;
            if (keys[SDL_SCANCODE_UP])    plane.y -= 5;
            if (keys[SDL_SCANCODE_DOWN])  plane.y += 5;

            // bounds
            if (plane.x < 0) plane.x = 0;
            if (plane.y < 0) plane.y = 0;
            if (plane.x > SCREEN_WIDTH - plane.w) plane.x = SCREEN_WIDTH - plane.w;
            if (plane.y > SCREEN_HEIGHT - plane.h) plane.y = SCREEN_HEIGHT - plane.h;

            // ---------------- SHOOT ----------------
            shootTimer++;
            int leftGun = plane.x + plane.w/4 - 8;
            int rightGun = plane.x + 3*plane.w/4 - 8;

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

            // ---------------- BULLETS ----------------
            for (int i = 0; i < MAX_BULLETS; i++)
            {
                if (bullets[i].active)
                {
                    bullets[i].y -= 8;
                    if (bullets[i].y < 0)
                        bullets[i].active = 0;
                }
            }

            // ---------------- ENEMIES ----------------
            for (int i = 0; i < MAX_ENEMIES; i++)
            {
                if (!enemies[i].active && rand()%1000 < 5)
                {
                    enemies[i].baseX = rand() % (SCREEN_WIDTH - 64);
                    enemies[i].x = enemies[i].baseX;
                    enemies[i].y = -64;
                    enemies[i].offset = (float)(rand()%360)/50.0f;
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

            // ---------------- COLLISIONS ----------------
            for (int i = 0; i < MAX_BULLETS; i++)
            for (int j = 0; j < MAX_ENEMIES; j++)
            {
                if (!bullets[i].active || !enemies[j].active) continue;

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
                if (!enemies[i].active) continue;

                SDL_Rect en = {(int)enemies[i].x, (int)enemies[i].y, 64, 64};

                if (SDL_HasIntersection(&plane, &en))
                {
                    enemies[i].active = 0;
                    playSound(hitSound);
                    lives--;

                    if (lives <= 0)
                        state = STATE_QUIT;
                }
            }

            // ---------------- RENDER GAME ----------------
            SDL_RenderCopy(renderer, planeTex, NULL, &plane);

            SDL_Rect r = {0,0,16,16};
            for (int i = 0; i < MAX_BULLETS; i++)
                if (bullets[i].active)
                {
                    r.x = bullets[i].x;
                    r.y = bullets[i].y;
                    SDL_RenderCopyEx(renderer, bulletTex, NULL, &r, -90, NULL, SDL_FLIP_NONE);
                }

            SDL_Rect er = {0,0,64,64};
            for (int i = 0; i < MAX_ENEMIES; i++)
                if (enemies[i].active)
                {
                    er.x = enemies[i].x;
                    er.y = enemies[i].y;
                    SDL_RenderCopyEx(renderer, enemyTex, NULL, &er, 0, NULL, SDL_FLIP_VERTICAL);
                }

            SDL_Rect hr = {10,10,32,32};
            for (int i = 0; i < lives; i++)
            {
                hr.x = 10 + i * 40;
                SDL_RenderCopy(renderer, heartTex, NULL, &hr);
            }
        }

        // ================= PAUSE MENU =================
        else if (state == STATE_PAUSE_MENU)
        {
            renderMenu(renderer, pauseMenuButtons, 2, font, 400, 600);
        }

        // ================= QUIT =================
        else if (state == STATE_QUIT)
        {
            running = false;
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    return 0;
}
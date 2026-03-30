#include <SDL2/SDL.h> 
#include <SDL2/SDL_image.h> 
#include <SDL2/SDL_ttf.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <time.h> 
#include <math.h> 
#include <stdbool.h> 
#include "../include/UI.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define MAX_BULLETS 100
#define MAX_ENEMIES 20
#define PLAYER_LIVES 4

typedef struct {
  float x, y;
  int active;
} Bullet;

typedef struct {
  float x, y;
  float baseX;
  float offset;
  int active;
} Enemy;

void handleButtonEvents(Button* b, SDL_Event* event, bool* quitFlag)
{
  int mouseX, mouseY;
  SDL_GetMouseState(&mouseX, &mouseY);

  b->isHovered = SDL_PointInRect(&(SDL_Point){mouseX, mouseY}, &b->rect);

  if (event->type == SDL_MOUSEBUTTONDOWN && b->isHovered) {
    if (event->button.button == SDL_BUTTON_LEFT) {
      *quitFlag = false; // quit the main loop
      exit(0);           // immediately quit
    }
  }
}
// Helper: load texture safely
SDL_Texture* loadTexture(SDL_Renderer* renderer, const char* path) {
  SDL_Surface* surf = IMG_Load(path);
  if (!surf) {
    printf("Failed to load %s: %s\n", path, IMG_GetError());
    return NULL;
  }

  SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
  SDL_FreeSurface(surf);

  if (!tex) {
    printf("Failed to create texture: %s\n", SDL_GetError());
  }

  return tex;
}

void handleKeyDown(SDL_Event* event, bool* menuOpen) {
  if (!event->key.repeat && event->key.keysym.sym == SDLK_ESCAPE) {
    *menuOpen = !(*menuOpen);
  }
}

int main(int argc, char* argv[]) {
  srand(time(NULL));

  // Init SDL
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    printf("SDL_Init Error: %s\n", SDL_GetError());
    return 1;
  }

  if (TTF_Init() != 0) {
    printf("TTF Init Error: %s\n", TTF_GetError());
    SDL_Quit();
    return 1;
  }

  if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
    printf("IMG_Init failed: %s\n", IMG_GetError());
    return 1;
  }

  SDL_Window* window = SDL_CreateWindow("Shooter",
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      SCREEN_WIDTH, SCREEN_HEIGHT, 0);

  if (!window) {
    printf("Window Error: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!renderer) {
    printf("Renderer Error: %s\n", SDL_GetError());
    return 1;
  }
  TTF_Font* font = TTF_OpenFont("resources/fonts/DejaVuSans.ttf", 24);
  if (!font) {
    printf("Failed to load font: %s\n", TTF_GetError());
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 1;
  }
  // Load textures
  SDL_Texture* planeTex  = loadTexture(renderer, "resources/plane1.png");
  SDL_Texture* bulletTex = loadTexture(renderer, "resources/test-bullet.png");
  SDL_Texture* enemyTex  = loadTexture(renderer, "resources/plane5.png");
  SDL_Texture* heartTex  = loadTexture(renderer, "resources/heart.png");

  SDL_Color topColor    = {0, 128, 255, 255};
  SDL_Color bottomColor = {0, 200, 255, 255};
  SDL_Color textColor   = {255, 255, 255, 255}; 

  Button quitButton = initializeButton(500, 60, topColor, bottomColor, textColor, "Quit");


  if (bulletTex)
    SDL_SetTextureBlendMode(bulletTex, SDL_BLENDMODE_BLEND);

  SDL_Rect plane = {SCREEN_WIDTH/2 - 32, SCREEN_HEIGHT - 80, 64, 64};

  Bullet bullets[MAX_BULLETS] = {0};
  Enemy enemies[MAX_ENEMIES] = {0};

  int score = 0;
  int lives = PLAYER_LIVES;
  int shootTimer = 0, shootDelay = 10;

  bool running = true;
  bool menuOpen = false;

  SDL_Event e;

  while (running) {
    // Events
    while (SDL_PollEvent(&e)) {
      switch (e.type) {
        case SDL_QUIT:
          running = false;
          break;

        case SDL_KEYDOWN:
          handleKeyDown(&e, &menuOpen);
          break;
        case SDL_MOUSEMOTION:
        case SDL_MOUSEBUTTONDOWN:
          if (menuOpen) handleButtonEvents(&quitButton, &e, &running);
          break;

      }
    }
    SDL_SetRenderDrawColor(renderer, 255,255,255,255);
    SDL_RenderClear(renderer);
    if(menuOpen) renderMenu(renderer, &quitButton, font);
    else {
      // Continuous input (FIXED)
      const Uint8* keys = SDL_GetKeyboardState(NULL);
      if (keys[SDL_SCANCODE_LEFT])  plane.x -= 5;
      if (keys[SDL_SCANCODE_RIGHT]) plane.x += 5;
      if (keys[SDL_SCANCODE_UP])    plane.y -= 5;
      if (keys[SDL_SCANCODE_DOWN])  plane.y += 5;

      // Bounds
      if (plane.x < 0) plane.x = 0;
      if (plane.x > SCREEN_WIDTH - plane.w) plane.x = SCREEN_WIDTH - plane.w;
      if (plane.y < 0) plane.y = 0;
      if (plane.y > SCREEN_HEIGHT - plane.h) plane.y = SCREEN_HEIGHT - plane.h;

      // Shooting
      shootTimer++;
      int leftGun = plane.x + plane.w/4 - 8;
      int rightGun = plane.x + 3*plane.w/4 - 8;

      if (shootTimer >= shootDelay) {
        shootTimer = 0;

        for (int i = 0; i < MAX_BULLETS; i++) {
          if (!bullets[i].active) {
            bullets[i].x = leftGun;
            bullets[i].y = plane.y;
            bullets[i].active = 1;
            break;
          }
        }

        for (int i = 0; i < MAX_BULLETS; i++) {
          if (!bullets[i].active) {
            bullets[i].x = rightGun;
            bullets[i].y = plane.y;
            bullets[i].active = 1;
            break;
          }
        }
      }

      // Update bullets
      for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
          bullets[i].y -= 8;
          if (bullets[i].y < 0)
            bullets[i].active = 0;
        }
      }

      // Spawn enemies
      for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active && rand()%1000 < 5) {
          enemies[i].baseX = rand() % (SCREEN_WIDTH - 64);
          enemies[i].x = enemies[i].baseX;
          enemies[i].y = -64;
          enemies[i].offset = (float)(rand()%360)/50.0f;
          enemies[i].active = 1;
        }
      }

      // Update enemies
      for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active) {
          enemies[i].y += 2;
          enemies[i].offset += 0.05f;
          enemies[i].x = enemies[i].baseX + 50 * sin(enemies[i].offset);

          if (enemies[i].y > SCREEN_HEIGHT)
            enemies[i].active = 0;
        }
      }

      // Bullet vs enemy
      for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;

        for (int j = 0; j < MAX_ENEMIES; j++) {
          if (!enemies[j].active) continue;

          SDL_Rect b = {(int)bullets[i].x, (int)bullets[i].y, 16, 16};
          SDL_Rect en = {(int)enemies[j].x, (int)enemies[j].y, 64, 64};

          if (SDL_HasIntersection(&b, &en)) {
            bullets[i].active = 0;
            enemies[j].active = 0;
            score += 10;
            printf("Score: %d\n", score);
          }
        }
      }

      // Enemy vs player
      for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;

        SDL_Rect en = {(int)enemies[i].x, (int)enemies[i].y, 64, 64};

        if (SDL_HasIntersection(&plane, &en)) {
          enemies[i].active = 0;
          lives--;

          printf("Lives: %d\n", lives);

          if (lives <= 0) {
            printf("Game Over!\n");
            running = false;
          }
        }
      }

      // Render

      if (planeTex)
        SDL_RenderCopy(renderer, planeTex, NULL, &plane);

      SDL_Rect bRect = {0,0,16,16};
      for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active && bulletTex) {
          bRect.x = (int)bullets[i].x;
          bRect.y = (int)bullets[i].y;
          SDL_RenderCopyEx(renderer, bulletTex, NULL, &bRect, -90, NULL, SDL_FLIP_NONE);
        }
      }

      SDL_Rect eRect = {0,0,64,64};
      for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active && enemyTex) {
          eRect.x = (int)enemies[i].x;
          eRect.y = (int)enemies[i].y;
          SDL_RenderCopyEx(renderer, enemyTex, NULL, &eRect, 0, NULL, SDL_FLIP_VERTICAL);
        }
      }

      SDL_Rect hRect = {10,10,32,32};
      for (int i = 0; i < lives; i++) {
        if (heartTex) {
          hRect.x = 10 + i * 40;
          SDL_RenderCopy(renderer, heartTex, NULL, &hRect);
        }
      }
    }

    SDL_RenderPresent(renderer);
    SDL_Delay(16);
  }

  SDL_DestroyTexture(planeTex);
  SDL_DestroyTexture(bulletTex);
  SDL_DestroyTexture(enemyTex);
  SDL_DestroyTexture(heartTex);

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  IMG_Quit();
  SDL_Quit();

  return 0;
}


#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define MAX_BULLETS 100
#define MAX_ENEMIES 20
#define PLAYER_LIVES 3

typedef struct {
    float x, y;
    int active;
    int fromBoss; // 0 = player bullet, 1 = boss bullet
} Bullet;

typedef struct {
    float x, y;
    int active;
} Enemy;

int main(int argc, char* argv[]) {
    srand(time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL init error: %s\n", SDL_GetError());
        return 1;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        printf("IMG init error: %s\n", IMG_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Shooter",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT, 0);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Load textures
    SDL_Texture* bgTex = SDL_CreateTextureFromSurface(renderer, IMG_Load("resources/map.png"));
    SDL_Texture* planeTex = SDL_CreateTextureFromSurface(renderer, IMG_Load("resources/plane1.png"));
    SDL_Texture* bulletTex = SDL_CreateTextureFromSurface(renderer, IMG_Load("resources/test-bullet.png"));
    SDL_Texture* enemyTex = SDL_CreateTextureFromSurface(renderer, IMG_Load("resources/plane4.png"));
    SDL_Texture* heartTex = SDL_CreateTextureFromSurface(renderer, IMG_Load("resources/heart.png"));

    SDL_SetTextureBlendMode(bulletTex, SDL_BLENDMODE_BLEND);

    // Background scroll variables
    float bgY1 = 0;
    float bgY2 = -SCREEN_HEIGHT;
    float bgSpeed = 0.5f;

    SDL_Rect plane = {SCREEN_WIDTH/2 - 32, SCREEN_HEIGHT - 80, 64, 64};
    Bullet bullets[MAX_BULLETS] = {0};
    Enemy enemies[MAX_ENEMIES] = {0};

    int score = 0;
    int lives = PLAYER_LIVES;
    int shootTimer = 0;
    int running = 1;
    SDL_Event e;

    while (running) {
        while (SDL_PollEvent(&e)) if (e.type == SDL_QUIT) running = 0;

        const Uint8* keys = SDL_GetKeyboardState(NULL);

        // Plane movement
        if (keys[SDL_SCANCODE_LEFT]) plane.x -= 5;
        if (keys[SDL_SCANCODE_RIGHT]) plane.x += 5;
        if (keys[SDL_SCANCODE_UP]) plane.y -= 5;
        if (keys[SDL_SCANCODE_DOWN]) plane.y += 5;

        if (plane.x < 0) plane.x = 0;
        if (plane.x > SCREEN_WIDTH - plane.w) plane.x = SCREEN_WIDTH - plane.w;
        if (plane.y < 0) plane.y = 0;
        if (plane.y > SCREEN_HEIGHT - plane.h) plane.y = SCREEN_HEIGHT - plane.h;

        // Scroll background
        bgY1 += bgSpeed;
        bgY2 += bgSpeed;
        if (bgY1 >= SCREEN_HEIGHT) bgY1 = bgY2 - SCREEN_HEIGHT;
        if (bgY2 >= SCREEN_HEIGHT) bgY2 = bgY1 - SCREEN_HEIGHT;

        // Shooting (auto-fire)
        shootTimer++;
        if (shootTimer >= 10) {
            shootTimer = 0;
            for(int i=0;i<MAX_BULLETS;i++){
                if (!bullets[i].active){
                    bullets[i].x = plane.x + plane.w/2 - 8;
                    bullets[i].y = plane.y;
                    bullets[i].active = 1;
                    bullets[i].fromBoss = 0;
                    break;
                }
            }
        }

        // Update bullets
        for(int i=0;i<MAX_BULLETS;i++){
            if (!bullets[i].active) continue;
            bullets[i].y += bullets[i].fromBoss ? 5 : -8;
            if (bullets[i].y < 0 || bullets[i].y > SCREEN_HEIGHT) bullets[i].active = 0;
        }

        // Spawn enemies
        for(int i=0;i<MAX_ENEMIES;i++){
            if (!enemies[i].active && rand()%1000<5){
                enemies[i].x = rand()%(SCREEN_WIDTH-64);
                enemies[i].y = -64;
                enemies[i].active = 1;
            }
        }

        // Update enemies
        for(int i=0;i<MAX_ENEMIES;i++){
            if (!enemies[i].active) continue;
            enemies[i].y += 2;
            if (enemies[i].y > SCREEN_HEIGHT) enemies[i].active = 0;
        }

        // Check collisions: bullets vs enemies
        for(int i=0;i<MAX_BULLETS;i++){
            if (!bullets[i].active || bullets[i].fromBoss) continue;
            SDL_Rect b = {(int)bullets[i].x, (int)bullets[i].y, 16, 16};
            for(int j=0;j<MAX_ENEMIES;j++){
                if (!enemies[j].active) continue;
                SDL_Rect en = {(int)enemies[j].x, (int)enemies[j].y, 64, 64};
                if (SDL_HasIntersection(&b, &en)){
                    bullets[i].active = 0;
                    enemies[j].active = 0;
                    score += 10;
                    printf("Score: %d\n", score);
                }
            }
        }

        // Check enemies vs plane
        SDL_Rect playerRect = plane;
        for(int i=0;i<MAX_ENEMIES;i++){
            if (!enemies[i].active) continue;
            SDL_Rect en = {(int)enemies[i].x,(int)enemies[i].y,64,64};
            if (SDL_HasIntersection(&playerRect,&en)){
                enemies[i].active = 0;
                lives--;
                printf("Lives: %d\n", lives);
                if (lives <= 0){
                    printf("Game Over!\n");
                    running = 0;
                }
            }
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 255,255,255,255);
        SDL_RenderClear(renderer);

        // Draw background
        SDL_Rect bg1 = {0, (int)bgY1, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_Rect bg2 = {0, (int)bgY2, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_RenderCopy(renderer, bgTex, NULL, &bg1);
        SDL_RenderCopy(renderer, bgTex, NULL, &bg2);

        // Draw plane
        SDL_RenderCopy(renderer, planeTex, NULL, &plane);

        // Draw bullets
        SDL_Rect bRect = {0,0,16,16};
        for(int i=0;i<MAX_BULLETS;i++){
            if (!bullets[i].active) continue;
            bRect.x = bullets[i].x;
            bRect.y = bullets[i].y;
            SDL_RenderCopy(renderer, bulletTex,NULL,&bRect);
        }

        // Draw enemies (vända 180°)
        SDL_Rect eRect = {0,0,64,64};
        for(int i=0;i<MAX_ENEMIES;i++){
            if (!enemies[i].active) continue;
            eRect.x = (int)enemies[i].x;
            eRect.y = (int)enemies[i].y;
            SDL_RenderCopyEx(renderer, enemyTex,NULL,&eRect,180,NULL,SDL_FLIP_NONE);
        }

        // Draw hearts
        SDL_Rect hRect = {10,10,32,32};
        for(int i=0;i<lives;i++){
            hRect.x = 10 + i*40;
            SDL_RenderCopy(renderer, heartTex,NULL,&hRect);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyTexture(bgTex);
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
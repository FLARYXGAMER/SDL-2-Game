#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define MAX_BULLETS 100
#define MAX_ENEMIES 20

typedef struct {
    float x, y;
    int active;
} Bullet;

typedef struct {
    float x, y;
    int active;
} Enemy;

int main(int argc, char* argv[]) {
    srand(time(NULL));

    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);

    SDL_Window* window = SDL_CreateWindow("Shooter Prototype",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT, 0);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Ladda textures
    SDL_Texture* planeTex = SDL_CreateTextureFromSurface(renderer, IMG_Load("resources/plane1.png"));
    SDL_Texture* bulletTex = SDL_CreateTextureFromSurface(renderer, IMG_Load("resources/test-bullet.png"));
    SDL_SetTextureBlendMode(bulletTex, SDL_BLENDMODE_BLEND);
    SDL_Texture* enemyTex = SDL_CreateTextureFromSurface(renderer, IMG_Load("resources/plane5.png"));

    // Plane
    SDL_Rect plane = {SCREEN_WIDTH/2 - 32, SCREEN_HEIGHT - 80, 64, 64};

    // Bullets
    Bullet bullets[MAX_BULLETS] = {0};

    // Enemies
    Enemy enemies[MAX_ENEMIES] = {0};

    int score = 0;
    int shootTimer = 0, shootDelay = 10;
    int running = 1;
    SDL_Event e;

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
        }

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

        // Auto-fire
        shootTimer++;
        int leftGun = plane.x + plane.w/4 - 8;
        int rightGun = plane.x + 3*plane.w/4 - 8;

        if (shootTimer >= shootDelay) {
            shootTimer = 0;
            for (int i=0;i<MAX_BULLETS;i++){
                if (!bullets[i].active){ bullets[i].x=leftGun; bullets[i].y=plane.y; bullets[i].active=1; break;}
            }
            for (int i=0;i<MAX_BULLETS;i++){
                if (!bullets[i].active){ bullets[i].x=rightGun; bullets[i].y=plane.y; bullets[i].active=1; break;}
            }
        }

        // Update bullets
        for(int i=0;i<MAX_BULLETS;i++){
            if (bullets[i].active){
                bullets[i].y -= 8;
                if (bullets[i].y<0) bullets[i].active=0;
            }
        }

        // Spawn enemies randomly
        for(int i=0;i<MAX_ENEMIES;i++){
            if (!enemies[i].active && rand()%1000<5){
                enemies[i].x = rand() % (SCREEN_WIDTH - 64);
                enemies[i].y = -64;
                enemies[i].active = 1;
            }
        }

        // Update enemies
        for(int i=0;i<MAX_ENEMIES;i++){
            if (enemies[i].active){
                enemies[i].y += 2;
                if (enemies[i].y > SCREEN_HEIGHT) enemies[i].active = 0;
            }
        }

        // Check collisions bullets vs enemies
        for(int i=0;i<MAX_BULLETS;i++){
            if (!bullets[i].active) continue;
            for(int j=0;j<MAX_ENEMIES;j++){
                if (!enemies[j].active) continue;
                SDL_Rect b = {(int)bullets[i].x,(int)bullets[i].y,16,16};
                SDL_Rect en = {(int)enemies[j].x,(int)enemies[j].y,64,64};
                if (SDL_HasIntersection(&b,&en)){
                    bullets[i].active=0;
                    enemies[j].active=0;
                    score+=10;
                    printf("Score: %d\n", score); // Visar score i terminalen
                }
            }
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 255,255,255,255);
        SDL_RenderClear(renderer);

        // Draw plane
        SDL_RenderCopy(renderer, planeTex,NULL,&plane);

        // Draw bullets
        SDL_Rect bRect={0,0,16,16};
        for(int i=0;i<MAX_BULLETS;i++){
            if (bullets[i].active){
                bRect.x = bullets[i].x;
                bRect.y = bullets[i].y;
                SDL_RenderCopyEx(renderer, bulletTex,NULL,&bRect,-90,NULL,SDL_FLIP_NONE);
            }
        }

        // Draw enemies
        SDL_Rect eRect={0,0,64,64};
        for(int i=0;i<MAX_ENEMIES;i++){
            if (enemies[i].active){
                eRect.x=(int)enemies[i].x;
                eRect.y=(int)enemies[i].y;
                SDL_RenderCopyEx(renderer, enemyTex, NULL, &eRect, 0, NULL, SDL_FLIP_VERTICAL);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyTexture(planeTex);
    SDL_DestroyTexture(bulletTex);
    SDL_DestroyTexture(enemyTex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    IMG_Quit();
    SDL_Quit();

    return 0;
}
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
    float baseX;
    float offset;
    int active;
} Enemy;

typedef struct {
    float x, y;
    int active;
    int shootTimer;
    int direction; // 1 = right, -1 = left
    int health;
} Boss;

int main(int argc, char* argv[]) {
    srand(time(NULL));

    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);

    SDL_Window* window = SDL_CreateWindow("Shooter with Boss",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT, 0);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Ladda textures
    SDL_Texture* planeTex = SDL_CreateTextureFromSurface(renderer, IMG_Load("resources/plane1.png"));
    SDL_Texture* bulletTex = SDL_CreateTextureFromSurface(renderer, IMG_Load("resources/test-bullet.png"));
    SDL_SetTextureBlendMode(bulletTex, SDL_BLENDMODE_BLEND);
    SDL_Texture* enemyTex = SDL_CreateTextureFromSurface(renderer, IMG_Load("resources/plane4.png"));
    SDL_Texture* heartTex = SDL_CreateTextureFromSurface(renderer, IMG_Load("resources/heart.png"));
    SDL_Texture* bossTex = SDL_CreateTextureFromSurface(renderer, IMG_Load("resources/plane5.png"));
    SDL_Texture* bossBulletTex = SDL_CreateTextureFromSurface(renderer, IMG_Load("resources/plane2.png"));

    SDL_Rect plane = {SCREEN_WIDTH/2 - 32, SCREEN_HEIGHT - 80, 64, 64};
    Bullet bullets[MAX_BULLETS] = {0};
    Enemy enemies[MAX_ENEMIES] = {0};
    Boss boss = {SCREEN_WIDTH/2 - 64, 50, 0, 0, 1, 200};
    int bossActive = 0;

    int score = 0;
    int lives = PLAYER_LIVES;
    int shootTimer = 0, shootDelay = 10;
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

        // Auto-fire
        shootTimer++;
        int leftGun = plane.x + plane.w/4 - 8;
        int rightGun = plane.x + 3*plane.w/4 - 8;

        if (shootTimer >= shootDelay) {
            shootTimer = 0;
            for (int i=0;i<MAX_BULLETS;i++){
                if (!bullets[i].active){ bullets[i].x=leftGun; bullets[i].y=plane.y; bullets[i].active=1; bullets[i].fromBoss=0; break;}
            }
            for (int i=0;i<MAX_BULLETS;i++){
                if (!bullets[i].active){ bullets[i].x=rightGun; bullets[i].y=plane.y; bullets[i].active=1; bullets[i].fromBoss=0; break;}
            }
        }

        // Update bullets
        for(int i=0;i<MAX_BULLETS;i++){
            if (!bullets[i].active) continue;
            bullets[i].y += bullets[i].fromBoss?5:-8;
            if (bullets[i].y < 0 || bullets[i].y > SCREEN_HEIGHT) bullets[i].active = 0;
        }

        // Spawn small enemies until boss
        if (!bossActive) {
            for(int i=0;i<MAX_ENEMIES;i++){
                if (!enemies[i].active && rand()%1000<5){
                    enemies[i].baseX = rand() % (SCREEN_WIDTH - 64);
                    enemies[i].x = enemies[i].baseX;
                    enemies[i].y = -64;
                    enemies[i].offset = (float)(rand()%360)/50.0f;
                    enemies[i].active = 1;
                }
            }
            // Update small enemies
            for(int i=0;i<MAX_ENEMIES;i++){
                if (!enemies[i].active) continue;
                enemies[i].y += 2;
                enemies[i].offset += 0.05f;
                enemies[i].x = enemies[i].baseX + 50 * sin(enemies[i].offset);
                if (enemies[i].y > SCREEN_HEIGHT) enemies[i].active = 0;
            }
        }

        // Activate boss at score 1000
        if (score >= 1000 && !bossActive) { bossActive = 1; boss.active = 1; printf("Boss fight!\n"); }

        // Update boss
        if (bossActive && boss.active) {
            boss.x += boss.direction * 3;
            if (boss.x < 0 || boss.x > SCREEN_WIDTH - 128) boss.direction *= -1;

            boss.shootTimer++;
            if (boss.shootTimer >= 50) { // Boss bullets
                boss.shootTimer = 0;
                for(int i=0;i<MAX_BULLETS;i++){
                    if (!bullets[i].active){
                        bullets[i].x = boss.x + 64 - 8;
                        bullets[i].y = boss.y + 128;
                        bullets[i].active = 1;
                        bullets[i].fromBoss = 1;
                        break;
                    }
                }
            }
        }

        // Check collisions bullets vs enemies
        for(int i=0;i<MAX_BULLETS;i++){
            if (!bullets[i].active || bullets[i].fromBoss) continue;

            for(int j=0;j<MAX_ENEMIES;j++){
                if (!enemies[j].active) continue;
                SDL_Rect b = {(int)bullets[i].x,(int)bullets[i].y,16,16};
                SDL_Rect en = {(int)enemies[j].x,(int)enemies[j].y,64,64};
                if (SDL_HasIntersection(&b,&en)){
                    bullets[i].active=0;
                    enemies[j].active=0;
                    score+=10;
                    printf("Score: %d\n", score);
                }
            }

            if (bossActive && boss.active){
                SDL_Rect b = {(int)bullets[i].x,(int)bullets[i].y,16,16};
                SDL_Rect be = {(int)boss.x,(int)boss.y,128,128};
                if (SDL_HasIntersection(&b,&be)){
                    bullets[i].active=0;
                    boss.health -= 10;
                    printf("Boss Health: %d\n", boss.health);
                    if (boss.health <= 0){
                        boss.active = 0;
                        printf("You Win!\n");
                        running = 0;
                    }
                }
            }
        }

        // Check boss bullets vs player
        for(int i=0;i<MAX_BULLETS;i++){
            if (!bullets[i].active || !bullets[i].fromBoss) continue;
            SDL_Rect b = {(int)bullets[i].x,(int)bullets[i].y,16,16};
            if (SDL_HasIntersection(&b,&plane)){
                bullets[i].active=0;
                lives--;
                printf("Lives: %d\n", lives);
                if (lives <= 0){
                    printf("Game Over!\n");
                    running = 0;
                }
            }
        }

        // Check small enemies vs player
        if (!bossActive){
            for(int i=0;i<MAX_ENEMIES;i++){
                if (!enemies[i].active) continue;
                SDL_Rect en = {(int)enemies[i].x,(int)enemies[i].y,64,64};
                if (SDL_HasIntersection(&plane,&en)){
                    enemies[i].active=0;
                    lives--;
                    printf("Lives: %d\n", lives);
                    if (lives <= 0){
                        printf("Game Over!\n");
                        running = 0;
                    }
                }
            }
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 255,255,255,255);
        SDL_RenderClear(renderer);

        SDL_RenderCopy(renderer, planeTex,NULL,&plane);

        SDL_Rect bRect={0,0,16,16};
        for(int i=0;i<MAX_BULLETS;i++){
            if (!bullets[i].active) continue;
            bRect.x = bullets[i].x;
            bRect.y = bullets[i].y;
            SDL_RenderCopyEx(renderer, bullets[i].fromBoss?bossBulletTex:bulletTex,NULL,&bRect,
                             bullets[i].fromBoss?90:-90,NULL,SDL_FLIP_NONE);
        }

        SDL_Rect eRect={0,0,64,64};
        if (!bossActive){
            for(int i=0;i<MAX_ENEMIES;i++){
                if (!enemies[i].active) continue;
                eRect.x = (int)enemies[i].x;
                eRect.y = (int)enemies[i].y;
                SDL_RenderCopyEx(renderer, enemyTex,NULL,&eRect,0,NULL,SDL_FLIP_VERTICAL);
            }
        }

        if (bossActive && boss.active){
            SDL_Rect bossRect = {(int)boss.x,(int)boss.y,128,128};
            SDL_RenderCopyEx(renderer, bossTex,NULL,&bossRect,0,NULL,SDL_FLIP_VERTICAL);
        }

        SDL_Rect hRect = {10,10,32,32};
        for(int i=0;i<lives;i++){
            hRect.x = 10 + i*40;
            SDL_RenderCopy(renderer, heartTex,NULL,&hRect);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyTexture(planeTex);
    SDL_DestroyTexture(bulletTex);
    SDL_DestroyTexture(enemyTex);
    SDL_DestroyTexture(heartTex);
    SDL_DestroyTexture(bossTex);
    SDL_DestroyTexture(bossBulletTex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    IMG_Quit();
    SDL_Quit();

    return 0;
}
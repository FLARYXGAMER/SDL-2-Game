#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define MAX_BULLETS 200
#define MAX_ENEMIES 30
#define PLAYER_LIVES 3

typedef struct {
    float x, y;
    float vx, vy;
    int active;
    int fromBoss; 
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
    int direction;
    int health;
} Boss;

// Funktion för fiende-mönster
void spawnPattern(Enemy enemies[], int patternType){
    int startX=50;
    switch(patternType){
        case 0: // linje
            for(int i=0;i<5;i++){
                for(int j=0;j<MAX_ENEMIES;j++){
                    if(!enemies[j].active){
                        enemies[j].x=startX+i*120;
                        enemies[j].y=-i*60;
                        enemies[j].baseX=enemies[j].x;
                        enemies[j].offset=(float)(rand()%360)/50.0f;
                        enemies[j].active=1;
                        break;
                    }
                }
            } break;
        case 1: // V-form
            for(int i=0;i<3;i++){
                for(int j=0;j<MAX_ENEMIES;j++){
                    if(!enemies[j].active){
                        enemies[j].x=startX+100*i;
                        enemies[j].y=-i*60;
                        enemies[j].baseX=enemies[j].x+50*i;
                        enemies[j].offset=(float)(rand()%360)/50.0f;
                        enemies[j].active=1;
                        break;
                    }
                }
            } break;
        case 2: // zigzag
            for(int i=0;i<5;i++){
                for(int j=0;j<MAX_ENEMIES;j++){
                    if(!enemies[j].active){
                        enemies[j].x=startX+i*100;
                        enemies[j].y=-i*50;
                        enemies[j].baseX=enemies[j].x;
                        enemies[j].offset=(i%2==0?0.1f:-0.1f);
                        enemies[j].active=1;
                        break;
                    }
                }
            } break;
    }
}

int main(int argc, char* argv[]){
    srand(time(NULL));

    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);

    SDL_Window* window=SDL_CreateWindow("Shooter Levels & Bosses", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    SDL_Renderer* renderer=SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Texture* bgTex = SDL_CreateTextureFromSurface(renderer, IMG_Load("resources/map.png"));
    SDL_Texture* planeTex = SDL_CreateTextureFromSurface(renderer, IMG_Load("resources/plane1.png"));
    SDL_Texture* bulletTex = SDL_CreateTextureFromSurface(renderer, IMG_Load("resources/test-bullet.png"));
    SDL_Texture* enemyTex = SDL_CreateTextureFromSurface(renderer, IMG_Load("resources/plane4.png"));
    SDL_Texture* heartTex = SDL_CreateTextureFromSurface(renderer, IMG_Load("resources/heart.png"));
    SDL_Texture* bossTex = SDL_CreateTextureFromSurface(renderer, IMG_Load("resources/plane5.png"));
    SDL_Texture* bossBulletTex = SDL_CreateTextureFromSurface(renderer, IMG_Load("resources/plane2.png"));
    SDL_SetTextureBlendMode(bulletTex, SDL_BLENDMODE_BLEND);

    float bgY1=0, bgY2=-SCREEN_HEIGHT, bgSpeed=0.5f;
    SDL_Rect plane={SCREEN_WIDTH/2-32, SCREEN_HEIGHT-80, 64,64};
    Bullet bullets[MAX_BULLETS]={0};
    Enemy enemies[MAX_ENEMIES]={0};
    Boss bosses[3]={{SCREEN_WIDTH/2-64,50,0,0,1,200},{SCREEN_WIDTH/2-64,50,0,0,1,300},{SCREEN_WIDTH/2-64,50,0,0,1,400}};
    int bossActive = -1; // -1 = inga bossar
    int bossDefeated[3]={0,0,0}; // markera bossar som besegrade

    int score=0, lives=PLAYER_LIVES, shootTimer=0, weaponType=0;
    int running=1; SDL_Event e;

    while(running){
        while(SDL_PollEvent(&e)){
            if(e.type==SDL_QUIT) running=0;
            if(e.type==SDL_KEYDOWN && e.key.keysym.scancode==SDL_SCANCODE_X){
                weaponType=(weaponType+1)%2;
                printf("Weapon changed to %s\n", weaponType==0?"normal":"diagonal");
            }
        }

        const Uint8* keys=SDL_GetKeyboardState(NULL);
        if(keys[SDL_SCANCODE_LEFT]) plane.x-=5;
        if(keys[SDL_SCANCODE_RIGHT]) plane.x+=5;
        if(keys[SDL_SCANCODE_UP]) plane.y-=5;
        if(keys[SDL_SCANCODE_DOWN]) plane.y+=5;
        if(plane.x<0) plane.x=0;
        if(plane.x>SCREEN_WIDTH-plane.w) plane.x=SCREEN_WIDTH-plane.w;
        if(plane.y<0) plane.y=0;
        if(plane.y>SCREEN_HEIGHT-plane.h) plane.y=SCREEN_HEIGHT-plane.h;

        // Bakgrund scroll
        bgY1+=bgSpeed; bgY2+=bgSpeed;
        if(bgY1>=SCREEN_HEIGHT) bgY1=bgY2-SCREEN_HEIGHT;
        if(bgY2>=SCREEN_HEIGHT) bgY2=bgY1-SCREEN_HEIGHT;

        // Skott
        shootTimer++;
        if(shootTimer>=10){
            shootTimer=0;
            if(weaponType==0){
                for(int i=0;i<MAX_BULLETS;i++){
                    if(!bullets[i].active){
                        bullets[i].x=plane.x+plane.w/2-8; bullets[i].y=plane.y; bullets[i].vx=0; bullets[i].vy=-8;
                        bullets[i].active=1; bullets[i].fromBoss=0; break;
                    }
                }
            } else {
                for(int i=0;i<MAX_BULLETS;i++){
                    if(!bullets[i].active){ bullets[i].x=plane.x+plane.w/2-8; bullets[i].y=plane.y; bullets[i].vx=-4; bullets[i].vy=-8; bullets[i].active=1; bullets[i].fromBoss=0; break;}
                }
                for(int i=0;i<MAX_BULLETS;i++){
                    if(!bullets[i].active){ bullets[i].x=plane.x+plane.w/2-8; bullets[i].y=plane.y; bullets[i].vx=4; bullets[i].vy=-8; bullets[i].active=1; bullets[i].fromBoss=0; break;}
                }
            }
        }

        // Update bullets
        for(int i=0;i<MAX_BULLETS;i++){
            if(!bullets[i].active) continue;
            bullets[i].x+=bullets[i].vx; bullets[i].y+=bullets[i].vy;
            if(bullets[i].y<0||bullets[i].y>SCREEN_HEIGHT||bullets[i].x<0||bullets[i].x>SCREEN_WIDTH) bullets[i].active=0;
        }

        // Spawn små fiender bara om ingen boss aktiv
        if(bossActive==-1){
            if(rand()%1000<5) spawnPattern(enemies, rand()%3);
            for(int i=0;i<MAX_ENEMIES;i++){
                if(!enemies[i].active) continue;
                enemies[i].y+=2;
                enemies[i].offset+=0.05f;
                enemies[i].x=enemies[i].baseX+50*sin(enemies[i].offset);
                if(enemies[i].y>SCREEN_HEIGHT) enemies[i].active=0;
            }
        }

        // Aktivera bossar
        if(score>=500 && bossActive==-1 && !bossDefeated[0]){ bossActive=0; bosses[0].active=1; for(int i=0;i<MAX_ENEMIES;i++) enemies[i].active=0; printf("Boss 1 fight!\n"); }
        if(score>=1000 && bossActive==-1 && !bossDefeated[1]){ bossActive=1; bosses[1].active=1; for(int i=0;i<MAX_ENEMIES;i++) enemies[i].active=0; printf("Boss 2 fight!\n"); }
        if(score>=1500 && bossActive==-1 && !bossDefeated[2]){ bossActive=2; bosses[2].active=1; for(int i=0;i<MAX_ENEMIES;i++) enemies[i].active=0; printf("Boss 3 fight!\n"); }

        // Update boss
        if(bossActive>=0 && bosses[bossActive].active){
            Boss *b=&bosses[bossActive];
            b->x+=b->direction*3;
            if(b->x<0||b->x>SCREEN_WIDTH-128) b->direction*=-1;
            b->shootTimer++;
            if(b->shootTimer>=50){
                b->shootTimer=0;
                for(int i=0;i<MAX_BULLETS;i++){
                    if(!bullets[i].active){
                        bullets[i].x=b->x+64-8; bullets[i].y=b->y+128; bullets[i].vx=0; bullets[i].vy=5; bullets[i].active=1; bullets[i].fromBoss=1;
                        break;
                    }
                }
            }
        }

        // Kollisionshantering
        for(int i=0;i<MAX_BULLETS;i++){
            if(!bullets[i].active) continue;
            SDL_Rect b={(int)bullets[i].x,(int)bullets[i].y,16,16};
            if(!bullets[i].fromBoss){
                for(int j=0;j<MAX_ENEMIES;j++){
                    if(!enemies[j].active) continue;
                    SDL_Rect en={(int)enemies[j].x,(int)enemies[j].y,64,64};
                    if(SDL_HasIntersection(&b,&en)){ bullets[i].active=0; enemies[j].active=0; score+=10; printf("Score: %d\n",score);}
                }
                if(bossActive>=0 && bosses[bossActive].active){
                    SDL_Rect be={(int)bosses[bossActive].x,(int)bosses[bossActive].y,128,128};
                    if(SDL_HasIntersection(&b,&be)){
                        bullets[i].active=0; 
                        bosses[bossActive].health-=10; 
                        printf("Boss Health: %d\n",bosses[bossActive].health); 
                        if(bosses[bossActive].health<=0){
                            bosses[bossActive].active=0;
                            bossDefeated[bossActive]=1; 
                            bossActive=-1;
                            printf("Boss defeated! Small enemies returning...\n");
                        }
                    }
                }
            } else {
                SDL_Rect p={plane.x,plane.y,plane.w,plane.h};
                if(SDL_HasIntersection(&b,&p)){ bullets[i].active=0; lives--; printf("Lives: %d\n",lives); if(lives<=0){ printf("Game Over!\n"); running=0; } }
            }
        }

        // Små fiender vs spelare
        if(bossActive==-1){
            SDL_Rect p={plane.x,plane.y,plane.w,plane.h};
            for(int i=0;i<MAX_ENEMIES;i++){
                if(!enemies[i].active) continue;
                SDL_Rect en={(int)enemies[i].x,(int)enemies[i].y,64,64};
                if(SDL_HasIntersection(&p,&en)){ enemies[i].active=0; lives--; printf("Lives: %d\n",lives); if(lives<=0){ printf("Game Over!\n"); running=0; } }
            }
        }

        // Render
        SDL_SetRenderDrawColor(renderer,255,255,255,255);
        SDL_RenderClear(renderer);
        SDL_Rect bg1={0,(int)bgY1,SCREEN_WIDTH,SCREEN_HEIGHT};
        SDL_Rect bg2={0,(int)bgY2,SCREEN_WIDTH,SCREEN_HEIGHT};
        SDL_RenderCopy(renderer,bgTex,NULL,&bg1);
        SDL_RenderCopy(renderer,bgTex,NULL,&bg2);

        SDL_RenderCopy(renderer,planeTex,NULL,&plane);

        SDL_Rect bRect={0,0,16,16};
        for(int i=0;i<MAX_BULLETS;i++){
            if(!bullets[i].active) continue;
            bRect.x=bullets[i].x; bRect.y=bullets[i].y;
            SDL_RenderCopy(renderer,bullets[i].fromBoss?bossBulletTex:bulletTex,NULL,&bRect);
        }

        SDL_Rect eRect={0,0,64,64};
        for(int i=0;i<MAX_ENEMIES;i++){
            if(!enemies[i].active) continue;
            eRect.x=(int)enemies[i].x; eRect.y=(int)enemies[i].y;
            SDL_RenderCopyEx(renderer,enemyTex,NULL,&eRect,180,NULL,SDL_FLIP_NONE);
        }

        if(bossActive>=0 && bosses[bossActive].active){
            SDL_Rect bossRect={(int)bosses[bossActive].x,(int)bosses[bossActive].y,128,128};
            SDL_RenderCopyEx(renderer,bossTex,NULL,&bossRect,0,NULL,SDL_FLIP_VERTICAL);
        }

        SDL_Rect hRect={10,10,32,32};
        for(int i=0;i<lives;i++){ hRect.x=10+i*40; SDL_RenderCopy(renderer,heartTex,NULL,&hRect); }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyTexture(bgTex); SDL_DestroyTexture(planeTex);
    SDL_DestroyTexture(bulletTex); SDL_DestroyTexture(enemyTex);
    SDL_DestroyTexture(heartTex); SDL_DestroyTexture(bossTex);
    SDL_DestroyTexture(bossBulletTex);
    SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window);
    IMG_Quit(); SDL_Quit();
    return 0;
}
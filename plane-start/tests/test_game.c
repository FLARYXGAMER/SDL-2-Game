#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include "game.h"
#include "constants.h"

static int passes = 0, failures = 0;

#define ASSERT(cond, msg) do { \
    if (cond) { printf("[PASS] %s\n", msg); passes++; } \
    else       { printf("[FAIL] %s\n", msg); failures++; } \
} while (0)

static void test_reset_plane_position(void)
{
    SDL_Rect plane = {999, 999, 64, 64};
    Bullet bullets[MAX_BULLETS] = {0};
    Enemy  enemies[MAX_ENEMIES] = {0};
    int lives = 1, score = 0, timer = 0;

    resetLocalGame(&plane, bullets, enemies, &lives, &score, &timer);

    ASSERT(plane.x == SCREEN_WIDTH / 2 - 32, "plane x reset to center");
    ASSERT(plane.y == SCREEN_HEIGHT - 80,     "plane y reset to bottom");
}

static void test_reset_clears_bullets(void)
{
    SDL_Rect plane = {0, 0, 64, 64};
    Bullet bullets[MAX_BULLETS];
    Enemy  enemies[MAX_ENEMIES] = {0};
    int lives = 4, score = 0, timer = 0;

    memset(bullets, 0xFF, sizeof(bullets)); /* fill with garbage */
    resetLocalGame(&plane, bullets, enemies, &lives, &score, &timer);

    int any_active = 0;
    for (int i = 0; i < MAX_BULLETS; i++)
        if (bullets[i].active) any_active = 1;
    ASSERT(!any_active, "all bullets cleared after reset");
}

static void test_reset_clears_enemies(void)
{
    SDL_Rect plane = {0, 0, 64, 64};
    Bullet bullets[MAX_BULLETS] = {0};
    Enemy  enemies[MAX_ENEMIES];
    int lives = 4, score = 0, timer = 0;

    memset(enemies, 0xFF, sizeof(enemies)); /* fill with garbage */
    resetLocalGame(&plane, bullets, enemies, &lives, &score, &timer);

    int any_active = 0;
    for (int i = 0; i < MAX_ENEMIES; i++)
        if (enemies[i].active) any_active = 1;
    ASSERT(!any_active, "all enemies cleared after reset");
}

static void test_reset_lives_score_timer(void)
{
    SDL_Rect plane = {0, 0, 64, 64};
    Bullet bullets[MAX_BULLETS] = {0};
    Enemy  enemies[MAX_ENEMIES] = {0};
    int lives = 1, score = 500, timer = 99;

    resetLocalGame(&plane, bullets, enemies, &lives, &score, &timer);

    ASSERT(lives == PLAYER_LIVES, "lives reset to PLAYER_LIVES");
    ASSERT(score == 0,            "score reset to 0");
    ASSERT(timer == 0,            "shoot timer reset to 0");
}

int main(void)
{
    printf("=== Game Logic Tests ===\n");
    test_reset_plane_position();
    test_reset_clears_bullets();
    test_reset_clears_enemies();
    test_reset_lives_score_timer();

    printf("\n%d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

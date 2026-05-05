#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
    SDL_Rect      rect;
    SDL_Texture*  bgTexture;
    SDL_Color     textColor;
    bool          isHovered;
    const char*   label;
} Button;

#define MAIN_MENU_COUNT  4
#define PAUSE_MENU_COUNT 2
#define GAME_OVER_COUNT  2

typedef struct {
    Button mainMenu[MAIN_MENU_COUNT];
    Button pauseMenu[PAUSE_MENU_COUNT];
    Button gameOver[GAME_OVER_COUNT];
} MenuButtons;

void initMenuButtons(MenuButtons *menus, SDL_Texture *buttonTex);

Button    initializeButton(int w, int h, SDL_Texture* bg, SDL_Color text, const char* label);
void      centerButtonBottom(Button* b, SDL_Rect parent, int padding);
void      layoutButtons(Button* buttons, int count, SDL_Rect box, int startOffsetY, int spacing);
void      renderMenu(SDL_Renderer* renderer, Button* buttons, int count, TTF_Font* font, int sizeW, int sizeH, const char* title);
void      renderButton(SDL_Renderer* renderer, Button* b, TTF_Font* font);
void      drawCenterText(SDL_Renderer* renderer, TTF_Font* font, const char* text, int y, SDL_Color color);
void      renderGameOverMenu(SDL_Renderer* renderer, TTF_Font* font, Button* buttons, int count, int finalScore);
void      renderTextTopRight(SDL_Renderer* renderer, TTF_Font* font, const char* text, SDL_Color color);
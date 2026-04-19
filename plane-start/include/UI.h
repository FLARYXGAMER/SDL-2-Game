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

Button    initializeButton(int w, int h, SDL_Texture* bg, SDL_Color text, const char* label);
void      centerButtonBottom(Button* b, SDL_Rect parent, int padding);
void      layoutButtons(Button* buttons, int count, SDL_Rect box, int startOffsetY, int spacing);
void      renderMenu(SDL_Renderer* renderer, Button* buttons, int count, TTF_Font* font, int sizeW, int sizeH);
void      renderButton(SDL_Renderer* renderer, Button* b, TTF_Font* font);
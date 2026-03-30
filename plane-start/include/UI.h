#ifndef UI_H
#define UI_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>

typedef struct Button {
    SDL_Rect rect;
    SDL_Color topColor;
    SDL_Color bottomColor;
    SDL_Color textColor;
    bool isHovered;
    const char* label;
} Button;

// Function declarations
Button initializeButton(int w, int h, SDL_Color top, SDL_Color bottom, SDL_Color text, const char* label);
void centerButtonBottom(Button* b, SDL_Rect parent, int padding);
void renderMenu(SDL_Renderer* renderer, Button* button, TTF_Font* font);
void renderButton(SDL_Renderer* renderer, Button* b, TTF_Font* font);

#endif

#include "../include/UI.h"
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>  // for exit()

Button initializeButton(int w, int h, SDL_Color top, SDL_Color bottom, SDL_Color text, const char* label)
{
    Button b;
    b.rect.w = w;
    b.rect.h = h;
    b.rect.x = 0; // will be set by layout
    b.rect.y = 0;
    b.topColor = top;
    b.bottomColor = bottom;
    b.textColor = text;
    b.isHovered = false;
    b.label = label;
    return b;
}

// --- Layout helper: bottom center ---
void centerButtonBottom(Button* b, SDL_Rect parent, int padding)
{
    b->rect.x = parent.x + (parent.w - b->rect.w) / 2;
    b->rect.y = parent.y + parent.h - b->rect.h - padding;
}

// --- Render menu with button ---
void renderMenu(SDL_Renderer* renderer, Button* button, TTF_Font* font)
{
    int w, h;
    SDL_GetRendererOutputSize(renderer, &w, &h);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Overlay
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_Rect overlay = {0, 0, w, h};
    SDL_RenderFillRect(renderer, &overlay);

    // Menu box
    int baseW = 300;
    int baseH = 200;

    // Apply scaling
    int menuW = (int)(baseW * 2);
    int menuH = (int)(baseH * 2);

    // Center it
    SDL_Rect box = {
        w/2 - menuW/2,
        h/2 - menuH/2,
        menuW,
        menuH
    };
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &box);

    // Layout button at bottom
    centerButtonBottom(button, box, 20);

    renderButton(renderer, button, font);
}

// --- Render a button with vertical gradient and shadow ---
void renderButton(SDL_Renderer* renderer, Button* b, TTF_Font* font)
{
    // --- Draw shadow ---
    SDL_Rect shadowRect = b->rect;
    shadowRect.x += 4;
    shadowRect.y += 4;
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
    SDL_RenderFillRect(renderer, &shadowRect);

    // --- Draw gradient button ---
    SDL_Color top = b->topColor;
    SDL_Color bottom = b->bottomColor;
    int h = b->rect.h;
    
    Uint8 gradiant = bottom.r - top.r;
    for (int y = 0; y < h; y++) {
        float t = (float)y / h;
        Uint8 r = top.r + t * (bottom.r - top.r);
        Uint8 g = top.g + t * (bottom.g - top.g);
        Uint8 bl = top.b + t * (bottom.b - top.b);
        Uint8 a = top.a + t * (bottom.a - top.a);
        SDL_SetRenderDrawColor(renderer, r, g, bl, a);
        SDL_RenderDrawLine(renderer, b->rect.x, b->rect.y + y, b->rect.x + b->rect.w, b->rect.y + y); //Ritar en Horizontell linje
    }

    // --- Draw text ---
    SDL_Surface* surface = TTF_RenderText_Blended(font, b->label, b->textColor);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

    int textW = surface->w;
    int textH = surface->h;
    SDL_FreeSurface(surface);

    SDL_Rect textRect = {
        b->rect.x + (b->rect.w - textW)/2,
        b->rect.y + (b->rect.h - textH)/2,
        textW,
        textH
    };
    SDL_RenderCopy(renderer, texture, NULL, &textRect);
    SDL_DestroyTexture(texture);
}
// --- Handle hover and click ---

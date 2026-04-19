#include "../include/UI.h"
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>

Button initializeButton(int w, int h, SDL_Texture* bg, SDL_Color text, const char* label)
{
    Button b;
    b.rect.w = w;
    b.rect.h = h;
    b.rect.x = 0;
    b.rect.y = 0;
    b.bgTexture = bg;
    b.textColor = text;
    b.isHovered = false;
    b.label = label;
    return b;
}

void centerButtonBottom(Button* b, SDL_Rect parent, int padding)
{
    b->rect.x = parent.x + (parent.w - b->rect.w) / 2;
    b->rect.y = parent.y + parent.h - b->rect.h - padding;
}

void layoutButtons(Button* buttons, int count, SDL_Rect box, int startOffsetY, int spacing)
{
    for (int i = 0; i < count; i++)
    {
        buttons[i].rect.x = box.x + (box.w - buttons[i].rect.w) / 2;
        buttons[i].rect.y = box.y + startOffsetY + i * (buttons[i].rect.h + spacing);
    }
}

void drawMenuBackground(SDL_Renderer* renderer, int w, int h)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 128, 200);
    SDL_Rect overlay = {0, 0, w, h};
    SDL_RenderFillRect(renderer, &overlay);
}

void drawMenuBox(SDL_Renderer* renderer, SDL_Rect box)
{
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &box);
}

SDL_Rect createMenuBox(int screenW, int screenH, int sizeW, int sizeH)
{
    SDL_Rect box;
    box.w = sizeW;
    box.h = sizeH;
    box.x = screenW / 2 - box.w / 2;
    box.y = screenH / 2 - box.h / 2;
    return box;
}

void renderMenu(SDL_Renderer* renderer, Button buttons[], int count, TTF_Font* font, int sizeW, int sizeH)
{
    int w, h;
    SDL_GetRendererOutputSize(renderer, &w, &h);
    SDL_Rect box = createMenuBox(w, h, sizeW, sizeH);
    drawMenuBackground(renderer, w, h);
    drawMenuBox(renderer, box);
    layoutButtons(buttons, count, box, 80, 20);
    for (int i = 0; i < count; i++)
        renderButton(renderer, &buttons[i], font);
}

void renderButton(SDL_Renderer* renderer, Button* b, TTF_Font* font)
{
    // --- Shadow ---
    SDL_Rect shadowRect = b->rect;
    shadowRect.x += 4;
    shadowRect.y += 4;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
    SDL_RenderFillRect(renderer, &shadowRect);

    // --- Background texture ---
    if (b->bgTexture) {
        if (b->isHovered)
            SDL_SetTextureColorMod(b->bgTexture, 180, 180, 180);
        else
            SDL_SetTextureColorMod(b->bgTexture, 255, 255, 255);

        SDL_RenderCopy(renderer, b->bgTexture, NULL, &b->rect);
    } else {
        // Fallback: solid color if no texture loaded
        SDL_SetRenderDrawColor(renderer, 0, 128, 255, 255);
        SDL_RenderFillRect(renderer, &b->rect);
    }

    // --- Text ---
    SDL_Surface* surface = TTF_RenderText_Blended(font, b->label, b->textColor);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    int textW = surface->w;
    int textH = surface->h;
    SDL_FreeSurface(surface);

    SDL_Rect textRect = {
        b->rect.x + (b->rect.w - textW) / 2,
        b->rect.y + (b->rect.h - textH) / 2,
        textW,
        textH
    };
    SDL_RenderCopy(renderer, texture, NULL, &textRect);
    SDL_DestroyTexture(texture);
}
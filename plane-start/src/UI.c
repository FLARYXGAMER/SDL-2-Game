#include "../include/UI.h"
#include "constants.h"
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
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

void drawCenterText(SDL_Renderer *renderer, TTF_Font *font, const char *text, int y, SDL_Color color)
{
    SDL_Surface *surface = TTF_RenderText_Blended(font, text, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

    int w, h;
    SDL_QueryTexture(texture, NULL, NULL, &w, &h);

    SDL_Rect rect = {SCREEN_WIDTH / 2 - w / 2, y, w, h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void renderGameOverMenu(SDL_Renderer *renderer, TTF_Font *font, Button *buttons, int count, int finalScore)
{
    SDL_Color cyan = {0, 230, 255, 255};
    SDL_Color red  = {255, 70, 70, 255};

    SDL_Rect panel = {SCREEN_WIDTH / 2 - 360, 110, 720, 590};

    SDL_SetRenderDrawColor(renderer, 20, 25, 60, 255);
    SDL_RenderFillRect(renderer, &panel);

    SDL_SetRenderDrawColor(renderer, 0, 220, 255, 255);
    SDL_RenderDrawRect(renderer, &panel);

    drawCenterText(renderer, font, "GAME OVER", 170, red);

    char scoreText[128];
    sprintf(scoreText, "YOUR SCORE: %d", finalScore);
    drawCenterText(renderer, font, scoreText, 250, cyan);

    layoutButtons(buttons, count, panel, 270, 40);
    for (int i = 0; i < count; i++)
        renderButton(renderer, &buttons[i], font);
}

void initMenuButtons(MenuButtons *menus, SDL_Texture *buttonTex)
{
    SDL_Color white = {255, 255, 255, 255};
    int w = 200 * 1.5f;
    int h = 60  * 1.5f;

    menus->mainMenu[0] = initializeButton(w, h, buttonTex, white, "Start");
    menus->mainMenu[1] = initializeButton(w, h, buttonTex, white, "Quit");
    menus->mainMenu[2] = initializeButton(w, h, buttonTex, white, "Server");
    menus->mainMenu[3] = initializeButton(w, h, buttonTex, white, "Client");

    menus->pauseMenu[0] = initializeButton(w, h, buttonTex, white, "Resume");
    menus->pauseMenu[1] = initializeButton(w, h, buttonTex, white, "Quit");

    menus->gameOver[0] = initializeButton(w, h, buttonTex, white, "Play Again");
    menus->gameOver[1] = initializeButton(w, h, buttonTex, white, "Quit");
}

void renderTextTopRight(SDL_Renderer *renderer, TTF_Font *font, const char *text, SDL_Color color)
{
    SDL_Surface *surface = TTF_RenderText_Blended(font, text, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    int w, h;
    SDL_QueryTexture(texture, NULL, NULL, &w, &h);
    SDL_Rect rect = {SCREEN_WIDTH - w - 20, 10, w, h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
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
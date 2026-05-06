#include "../include/UI.h"
#include "constants.h"
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Button initializeButton(int w, int h, SDL_Texture *bg, SDL_Color text, const char *label)
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

void centerButtonBottom(Button *b, SDL_Rect parent, int padding)
{
    b->rect.x = parent.x + (parent.w - b->rect.w) / 2;
    b->rect.y = parent.y + parent.h - b->rect.h - padding;
}

void layoutButtons(Button *buttons, int count, SDL_Rect box, int startOffsetY, int spacing)
{
    for (int i = 0; i < count; i++)
    {
        buttons[i].rect.x = box.x + (box.w - buttons[i].rect.w) / 2;
        buttons[i].rect.y = box.y + startOffsetY + i * (buttons[i].rect.h + spacing);
    }
}

// ---------- internal drawing helpers ----------

static void rfill(SDL_Renderer *r, int x, int y, int w, int h,
                  Uint8 rv, Uint8 gv, Uint8 bv, Uint8 av)
{
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, rv, gv, bv, av);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderFillRect(r, &rect);
}

static void rborder(SDL_Renderer *r, int x, int y, int w, int h, int thick,
                    Uint8 rv, Uint8 gv, Uint8 bv, Uint8 av)
{
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, rv, gv, bv, av);
    for (int i = 0; i < thick; i++)
    {
        SDL_Rect rect = {x + i, y + i, w - 2 * i, h - 2 * i};
        SDL_RenderDrawRect(r, &rect);
    }
}

static void corners(SDL_Renderer *r, int x, int y, int w, int h, int sz,
                    Uint8 rv, Uint8 gv, Uint8 bv, Uint8 av)
{
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, rv, gv, bv, av);
    int x2 = x + w - 1, y2 = y + h - 1;
    SDL_RenderDrawLine(r, x, y, x + sz, y);
    SDL_RenderDrawLine(r, x, y, x, y + sz);
    SDL_RenderDrawLine(r, x2, y, x2 - sz, y);
    SDL_RenderDrawLine(r, x2, y, x2, y + sz);
    SDL_RenderDrawLine(r, x, y2, x + sz, y2);
    SDL_RenderDrawLine(r, x, y2, x, y2 - sz);
    SDL_RenderDrawLine(r, x2, y2, x2 - sz, y2);
    SDL_RenderDrawLine(r, x2, y2, x2, y2 - sz);
}

// Creates a texture from text; caller must SDL_DestroyTexture it.
static SDL_Texture *mktext(SDL_Renderer *r, TTF_Font *font, const char *text,
                           SDL_Color col, int *tw, int *th)
{
    SDL_Surface *surf = TTF_RenderText_Blended(font, text, col);
    if (!surf)
        return NULL;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
    *tw = surf->w;
    *th = surf->h;
    SDL_FreeSurface(surf);
    return tex;
}

// Render text with top-left at (x, y).
static void rtext(SDL_Renderer *r, TTF_Font *font, const char *text,
                  SDL_Color col, int x, int y)
{
    int tw, th;
    SDL_Texture *tex = mktext(r, font, text, col, &tw, &th);
    if (!tex)
        return;
    SDL_Rect dst = {x, y, tw, th};
    SDL_RenderCopy(r, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
}

// Render text horizontally centered at cx, top at topY.
static void rtextcx(SDL_Renderer *r, TTF_Font *font, const char *text,
                    SDL_Color col, int cx, int topY)
{
    int tw, th;
    SDL_Texture *tex = mktext(r, font, text, col, &tw, &th);
    if (!tex)
        return;
    SDL_Rect dst = {cx - tw / 2, topY, tw, th};
    SDL_RenderCopy(r, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
}

// Render text scaled up, horizontally centered; returns rendered height.
static int rtextscale(SDL_Renderer *r, TTF_Font *font, const char *text,
                      SDL_Color col, int cx, int topY, float scale)
{
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    int tw, th;
    SDL_Texture *tex = mktext(r, font, text, col, &tw, &th);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    if (!tex)
        return 0;
    int dw = (int)(tw * scale), dh = (int)(th * scale);
    SDL_Rect dst = {cx - dw / 2, topY, dw, dh};
    SDL_RenderCopy(r, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
    return dh;
}

// ---------- panel helpers ----------

static void drawPanel(SDL_Renderer *r, SDL_Rect p,
                      Uint8 fr, Uint8 fg, Uint8 fb,  // fill color
                      Uint8 br, Uint8 bg_, Uint8 bb) // border color
{
    rfill(r, p.x, p.y, p.w, p.h, fr, fg, fb, 248);
    rborder(r, p.x, p.y, p.w, p.h, 2, br, bg_, bb, 255);
    rborder(r, p.x + 6, p.y + 6, p.w - 12, p.h - 12, 1,
            br / 4, bg_ / 4, bb / 4, 140);
    corners(r, p.x + 3, p.y + 3, p.w - 6, p.h - 6, 18, 255, 165, 0, 255);
}

// ---------- public functions ----------

void drawMenuBackground(SDL_Renderer *renderer, int w, int h)
{
    rfill(renderer, 0, 0, w, h, 4, 8, 20, 218);
}

void drawMenuBox(SDL_Renderer *renderer, SDL_Rect box)
{
    drawPanel(renderer, box, 12, 20, 45, 0, 190, 240);
}

void renderMenu(SDL_Renderer *renderer, Button buttons[], int count,
                TTF_Font *font, int boxW, int boxH, const char *title)
{
    SDL_Rect box;

    box.w = boxW;
    box.h = boxH;

    // CENTRERA MENYRUTAN
    box.x = (SCREEN_WIDTH - box.w) / 2;
    box.y = (SCREEN_HEIGHT - box.h) / 2;

    // Rita halvtransparent bakgrundsruta
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
    SDL_RenderFillRect(renderer, &box);

    // Rita vit kant runt rutan
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 180);
    SDL_RenderDrawRect(renderer, &box);

    // Rita titel
    SDL_Color titleColor = {255, 255, 255, 255};
    SDL_Surface *titleSurface = TTF_RenderText_Blended(font, title, titleColor);

    if (titleSurface)
    {
        SDL_Texture *titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);

        SDL_Rect titleRect;
        titleRect.w = titleSurface->w;
        titleRect.h = titleSurface->h;
        titleRect.x = box.x + (box.w - titleRect.w) / 2;
        titleRect.y = box.y + 40;

        SDL_FreeSurface(titleSurface);

        SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
        SDL_DestroyTexture(titleTexture);
    }

    // KNAPPSTORLEK
    int buttonW = 220;
    int buttonH = 55;
    int spacing = 20;

    // Räkna ut total höjd för alla knappar
    int totalButtonsH = count * buttonH + (count - 1) * spacing;

    // Starta knapparna lite under titeln
    int startY = box.y + 130;

    // Om många knappar: centrera hela knappgruppen i boxen
    if (startY + totalButtonsH > box.y + box.h - 30)
    {
        startY = box.y + (box.h - totalButtonsH) / 2;
    }

    for (int i = 0; i < count; i++)
    {
        buttons[i].rect.w = buttonW;
        buttons[i].rect.h = buttonH;

        // DETTA ÄR DET VIKTIGA:
        // Knappen centreras i menyrutan
        buttons[i].rect.x = box.x + (box.w - buttonW) / 2;
        buttons[i].rect.y = startY + i * (buttonH + spacing);

        renderButton(renderer, &buttons[i], font);
    }
}

void drawCenterText(SDL_Renderer *renderer, TTF_Font *font, const char *text, int y, SDL_Color color)
{
    int tw, th;
    SDL_Texture *tex = mktext(renderer, font, text, color, &tw, &th);
    if (!tex)
        return;
    SDL_Rect rect = {SCREEN_WIDTH / 2 - tw / 2, y, tw, th};
    SDL_RenderCopy(renderer, tex, NULL, &rect);
    SDL_DestroyTexture(tex);
}

void renderGameOverMenu(SDL_Renderer *renderer, TTF_Font *font,
                        Button *buttons, int count, int finalScore)
{
    int pw = 540, ph = 350;
    SDL_Rect panel = {SCREEN_WIDTH / 2 - pw / 2, SCREEN_HEIGHT / 2 - ph / 2, pw, ph};
    int cx = panel.x + panel.w / 2;

    rfill(renderer, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0, 185);
    drawPanel(renderer, panel, 28, 6, 6, 210, 40, 40);

    int titleH = rtextscale(renderer, font, "GAME OVER",
                            (SDL_Color){255, 55, 55, 255}, cx, panel.y + 28, 2.2f);

    int sepY = panel.y + 28 + titleH + 10;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 210, 40, 40, 210);
    SDL_RenderDrawLine(renderer, panel.x + 22, sepY, panel.x + panel.w - 22, sepY);
    SDL_SetRenderDrawColor(renderer, 120, 20, 20, 100);
    SDL_RenderDrawLine(renderer, panel.x + 22, sepY + 2, panel.x + panel.w - 22, sepY + 2);

    char buf[64];
    sprintf(buf, "SCORE   %d", finalScore);
    int scoreH = rtextscale(renderer, font, buf,
                            (SDL_Color){0, 215, 255, 255}, cx, sepY + 16, 1.6f);

    int btnOff = sepY + 16 + scoreH + 24 - panel.y;
    layoutButtons(buttons, count, panel, btnOff, 14);
    for (int i = 0; i < count; i++)
        renderButton(renderer, &buttons[i], font);
}

void initMenuButtons(MenuButtons *menus, SDL_Texture *buttonTex)
{
    SDL_Color white = {255, 255, 255, 255};
    int w = 260, h = 52;

    menus->mainMenu[0] = initializeButton(w, h, buttonTex, white, "Start");
    menus->mainMenu[1] = initializeButton(w, h, buttonTex, white, "Quit");
    menus->mainMenu[2] = initializeButton(w, h, buttonTex, white, "Server");
    menus->mainMenu[3] = initializeButton(w, h, buttonTex, white, "Client");

    menus->pauseMenu[0] = initializeButton(w, h, buttonTex, white, "Resume");
    menus->pauseMenu[1] = initializeButton(w, h, buttonTex, white, "Quit");

    menus->gameOver[0] = initializeButton(w, h, buttonTex, white, "Play Again");
    menus->gameOver[1] = initializeButton(w, h, buttonTex, white, "Quit");
}

void renderTextTopRight(SDL_Renderer *renderer, TTF_Font *font,
                        const char *text, SDL_Color color)
{
    int tw, th;
    SDL_Texture *tex = mktext(renderer, font, text, color, &tw, &th);
    if (!tex)
        return;
    SDL_Rect rect = {SCREEN_WIDTH - tw - 20, 10, tw, th};
    SDL_RenderCopy(renderer, tex, NULL, &rect);
    SDL_DestroyTexture(tex);
}

void renderButton(SDL_Renderer *renderer, Button *b, TTF_Font *font)
{
    int x = b->rect.x, y = b->rect.y, w = b->rect.w, h = b->rect.h;

    // Drop shadow
    rfill(renderer, x + 4, y + 5, w, h, 0, 0, 0, 100);

    if (b->isHovered)
    {
        // Soft outer glow (orange/gold)
        rfill(renderer, x - 5, y - 5, w + 10, h + 10, 255, 165, 0, 14);
        rfill(renderer, x - 3, y - 3, w + 6, h + 6, 255, 165, 0, 28);
        rfill(renderer, x - 1, y - 1, w + 2, h + 2, 255, 165, 0, 50);
        // Button fill
        rfill(renderer, x, y, w, h, 28, 52, 90, 255);
        // Gold border
        rborder(renderer, x, y, w, h, 2, 255, 165, 0, 255);
        // Inner top sheen
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 255, 210, 120, 80);
        SDL_RenderDrawLine(renderer, x + 3, y + 2, x + w - 4, y + 2);
        // Left accent bar (gold)
        SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255);
    }
    else
    {
        // Button fill
        rfill(renderer, x, y, w, h, 16, 30, 58, 255);
        // Cyan border
        rborder(renderer, x, y, w, h, 2, 0, 190, 240, 255);
        // Inner top sheen
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 100, 200, 240, 45);
        SDL_RenderDrawLine(renderer, x + 3, y + 2, x + w - 4, y + 2);
        // Left accent bar (cyan)
        SDL_SetRenderDrawColor(renderer, 0, 190, 240, 220);
    }
    // Accent bar (2px wide strip on left edge)
    SDL_RenderDrawLine(renderer, x + 5, y + 7, x + 5, y + h - 8);
    SDL_RenderDrawLine(renderer, x + 6, y + 7, x + 6, y + h - 8);

    // Label
    SDL_Color textCol = b->isHovered
                            ? (SDL_Color){255, 255, 255, 255}
                            : (SDL_Color){190, 225, 255, 255};

    int tw, th;
    SDL_Texture *tex = mktext(renderer, font, b->label, textCol, &tw, &th);
    if (tex)
    {
        SDL_Rect dst = {x + (w - tw) / 2, y + (h - th) / 2, tw, th};
        SDL_RenderCopy(renderer, tex, NULL, &dst);
        SDL_DestroyTexture(tex);
    }
}

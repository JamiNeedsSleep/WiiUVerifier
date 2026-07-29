#pragma once
#ifndef JMC_DISPLAY
#define JMC_DISPLAY
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_ttf.h>
#include <cstring>
#include <cstdio>

/*
Shared SDL2 display + on-screen message system.

This replaces two things everywhere in the app:
  - WHBGfx, for screen init and the render loop.
  - The OS Notification module (the little popup toasts), for status text.
    Since those toasts are drawn by the system itself (not by our own GX2
    context), swapping them for text WE draw means the message is only
    actually visible during frames where SDL2 is doing the rendering -
    which is why this header also takes over the whole render loop, not
    just the messages.

UNVERIFIED ASSUMPTION (please confirm on real hardware):
nn::swkbd and nn::erreula are Nintendo system applets - Draw calls for them
must happen "inside a valid GX2 rendering context" per Nintendo's own docs.
This header assumes that this SDL2 Wii U port keeps one GX2 context bound
across the SDL_RenderClear -> (your draw calls) -> SDL_RenderPresent
sequence, and mirrors that same content to both the TV and the GamePad
screen - the same way it treats a single display on Switch/PC (it has no
separate BeginRenderTV/BeginRenderDRC concept the way WHBGfx did). If the
keyboard or error dialog doesn't show up on one or both screens, this
assumption is wrong, and those two loops need to go back to owning WHBGfx
directly the way they did before (see git history / previous revision).
*/

static SDL_Window* g_window = nullptr;
static SDL_Renderer* g_renderer = nullptr;
static SDL_GameController* g_controller = nullptr;
static TTF_Font* g_font = nullptr;
static char g_msgText[256] = "";
static Uint32 g_msgColor = 0xFFFFFFFFu; // white, SDL2_gfx 0xRRGGBBAA format

static void Display_Init()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
    if (TTF_Init() < 0)
    {
        printf("TTF_Init failed: %s\n", TTF_GetError());
    }
    g_font = TTF_OpenFont(
        "/vol/content/fonts/oldmono.ttf",
        24
    );
    if (!g_font)
    {
        printf("Font failed: %s\n", TTF_GetError());
    }
    g_window = SDL_CreateWindow(
    	"WiiUVerifier",
    	SDL_WINDOWPOS_CENTERED,
    	SDL_WINDOWPOS_CENTERED,
    	1280,
    	720,
    	SDL_WINDOW_SHOWN);
    g_renderer = g_window ? SDL_CreateRenderer(g_window, -1, SDL_RENDERER_SOFTWARE) : nullptr;
    g_controller = SDL_GameControllerOpen(0);
}

static void Display_Shutdown()
{
    if (g_controller) SDL_GameControllerClose(g_controller);
    if (g_renderer) SDL_DestroyRenderer(g_renderer);
    if (g_window) SDL_DestroyWindow(g_window);
    TTF_CloseFont(g_font);
    TTF_Quit();
    SDL_Quit();
}
void SDL_Reset()
{
    Display_Shutdown();
    Display_Init();
}
// Replaces NotificationModule_AddInfoNotification / AddErrorNotification.
// Stores the message; DrawMessage() (called every frame via EndFrame())
// keeps it on screen top-left until the next call replaces it.
static void ShowMessage(const char* text, bool isError = false)
{
    if (!text) return;
    strncpy(g_msgText, text, sizeof(g_msgText) - 1);
    g_msgText[sizeof(g_msgText) - 1] = '\0';
    g_msgColor = isError ? 0xFF4040FFu : 0xFFFFFFFFu; // red-ish / white
}

/*static void DrawMessage()
{
    if (!g_renderer || !g_msgText[0]) return;
    // stringColor() only draws a single line, so split on '\n' in case a
    // message has one (e.g. the error-dialog text does).
    char buf[256];
    strncpy(buf, g_msgText, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    int y = 8;
    char* line = strtok(buf, "\n");
    while (line) {
        stringColor(g_renderer, 8, y, line, g_msgColor);
        y += 10;
        line = strtok(nullptr, "\n");
    }
}*/
void DrawText(const char* text, int x, int y)
{
    SDL_Color white = {255,255,255,255};

    SDL_Surface* surface = TTF_RenderText_Blended(
        g_font,
        text,
        white
    );

    if (!surface)
    {
        printf("RenderText failed: %s\n", TTF_GetError());
        return;
    }

    SDL_Texture* texture =
        SDL_CreateTextureFromSurface(
            g_renderer,
            surface
        );

    SDL_Rect dst;
    dst.x = x;
    dst.y = y;
    dst.w = surface->w;
    dst.h = surface->h;

    SDL_FreeSurface(surface);

    SDL_RenderCopy(
        g_renderer,
        texture,
        nullptr,
        &dst
    );

    SDL_DestroyTexture(texture);
}
void DrawMessage()
{
    DrawText(g_msgText, 20, 20);
}

static void BeginFrame()
{
    if (!g_renderer) return;
    SDL_SetRenderDrawColor(g_renderer, 0, 0, 255, 255);
    SDL_RenderClear(g_renderer);
}

// Call after any GX2 applet overlays (swkbd/erreula Draw*) have been
// issued for this frame - message text is drawn on top of them.
static void EndFrame()
{
    if (!g_renderer) return;
    DrawMessage();
    SDL_RenderPresent(g_renderer);
}
#endif

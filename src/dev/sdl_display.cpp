// -----------------------------------------------------------
// sdl_display.cpp
// -----------------------------------------------------------
// SDL2 framebuffer viewer for SGI Octane SI graphics
// -----------------------------------------------------------

#include "sdl_display.h"
#include "framebuffer.h"
#include <SDL2/SDL.h>
#include <iostream>

SDLDisplay::SDLDisplay() {}
SDLDisplay::~SDLDisplay()
{
    if (texture)  SDL_DestroyTexture((SDL_Texture*)texture);
    if (renderer) SDL_DestroyRenderer((SDL_Renderer*)renderer);
    if (window)   SDL_DestroyWindow((SDL_Window*)window);
    SDL_Quit();
}

// -----------------------------------------------------------
// Initialize SDL2 window + texture
// -----------------------------------------------------------
bool SDLDisplay::init(uint32_t w, uint32_t h)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "[SDL] Init failed: " << SDL_GetError() << "\n";
        return false;
    }

    fb_width  = w;
    fb_height = h;

    window = SDL_CreateWindow(
        "Racer Emulator - SGI Octane1",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        w, h,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cerr << "[SDL] Window failed: " << SDL_GetError() << "\n";
        return false;
    }

    renderer = SDL_CreateRenderer((SDL_Window*)window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "[SDL] Renderer failed: " << SDL_GetError() << "\n";
        return false;
    }

    texture = SDL_CreateTexture(
        (SDL_Renderer*)renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        w, h
    );

    if (!texture) {
        std::cerr << "[SDL] Texture failed: " << SDL_GetError() << "\n";
        return false;
    }

    std::cout << "[SDL] Display initialized: " << w << "x" << h << "\n";

    return true;
}

// -----------------------------------------------------------
// Update SDL2 texture from emulator framebuffer
// -----------------------------------------------------------
void SDLDisplay::update(Framebuffer& fb)
{
    SDL_UpdateTexture(
        (SDL_Texture*)texture,
        nullptr,
        fb.data(),
        fb_width * 4
    );

    SDL_RenderClear((SDL_Renderer*)renderer);
    SDL_RenderCopy((SDL_Renderer*)renderer, (SDL_Texture*)texture, nullptr, nullptr);
    SDL_RenderPresent((SDL_Renderer*)renderer);
}

// -----------------------------------------------------------
// Handle window events
// -----------------------------------------------------------
void SDLDisplay::process_events(bool& quit)
{
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT)
            quit = true;

        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
            quit = true;
    }
}

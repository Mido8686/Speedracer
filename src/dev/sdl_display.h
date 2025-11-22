// -----------------------------------------------------------
// sdl_display.h
// -----------------------------------------------------------
// SDL2 display window for Racer Emulator framebuffer
// -----------------------------------------------------------

#pragma once
#include <cstdint>

class Framebuffer;

class SDLDisplay {
public:
    SDLDisplay();
    ~SDLDisplay();

    bool init(uint32_t w, uint32_t h);
    void update(Framebuffer& fb);
    void process_events(bool& quit);

private:
    void* window = nullptr;   // SDL_Window*
    void* renderer = nullptr; // SDL_Renderer*
    void* texture = nullptr;  // SDL_Texture*

    uint32_t fb_width  = 0;
    uint32_t fb_height = 0;
};

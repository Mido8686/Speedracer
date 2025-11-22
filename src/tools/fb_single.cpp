// fb_single.cpp
// Single-file SGI Octane framebuffer test + display (SDL2)
// Shows a 1280x1024 framebuffer (guest big-endian 0xAARRGGBB words).
// Keys:
//   ESC / close window -> quit
//   SPACE -> redraw test pattern
//   i -> toggle integer-scaling (pixel-perfect zoom)
//   v -> toggle vsync (re-creates renderer when changed)
//
// Build (Linux): g++ -O2 -std=c++17 fb_single.cpp `pkg-config --cflags --libs sdl2` -o fb_test
// Run: ./fb_test
//
// Integrate: copy FramebufferDevice::fill_test_pattern and conversion + rectangle logic into your emulator.

#include <SDL.h>
#include <chrono>
#include <thread>
#include <vector>
#include <cstdint>
#include <iostream>
#include <atomic>
#include <cstring>

static constexpr int FB_W = 1280;
static constexpr int FB_H = 1024;
static constexpr int FB_BPP = 4; // bytes per pixel
static constexpr int DEFAULT_PITCH = FB_W * FB_BPP; // bytes per scanline
static constexpr uint32_t NATIVE_ASPECT_NUM = 5; // 5:4 -> 1280:1024
static constexpr uint32_t NATIVE_ASPECT_DEN = 4;

struct GuestFramebuffer {
    // Simulated guest physical framebuffer memory (big-endian 32-bit words)
    std::vector<uint8_t> bytes; // size = pitch * height
    uint32_t width;
    uint32_t height;
    uint32_t pitch; // bytes per line

    GuestFramebuffer(uint32_t w=FB_W, uint32_t h=FB_H, uint32_t p=DEFAULT_PITCH)
      : width(w), height(h), pitch(p) {
        bytes.resize(static_cast<size_t>(pitch) * static_cast<size_t>(height));
    }

    // Write a 32-bit word into guest memory as big-endian
    inline void write_pixel_be(unsigned x, unsigned y, uint32_t pixel_be) {
        size_t idx = static_cast<size_t>(y) * pitch + static_cast<size_t>(x) * 4;
        bytes[idx + 0] = static_cast<uint8_t>((pixel_be >> 24) & 0xFF);
        bytes[idx + 1] = static_cast<uint8_t>((pixel_be >> 16) & 0xFF);
        bytes[idx + 2] = static_cast<uint8_t>((pixel_be >> 8) & 0xFF);
        bytes[idx + 3] = static_cast<uint8_t>((pixel_be >> 0) & 0xFF);
    }

    // Fill with test pattern (color bars + gradient)
    void fill_test_pattern() {
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                uint8_t r,g,b,a;
                a = 0xFF;
                if (x < width/8) { r=0xFF; g=0x00; b=0x00; }         // red
                else if (x < width*2/8) { r=0x00; g=0xFF; b=0x00; }  // green
                else if (x < width*3/8) { r=0x00; g=0x00; b=0xFF; }  // blue
                else if (x < width*4/8) { r=0xFF; g=0xFF; b=0x00; }  // yellow
                else if (x < width*5/8) { r=0xFF; g=0x00; b=0xFF; }  // magenta
                else if (x < width*6/8) { r=0x00; g=0xFF; b=0xFF; }  // cyan
                else if (x < width*7/8) { r = static_cast<uint8_t>((x*255)/width); g=0x40; b=0x40; } // ramp
                else { r = static_cast<uint8_t>((y*255)/height); g = r; b = r; } // gradient
                uint32_t pixel_be = (static_cast<uint32_t>(a) << 24) |
                                    (static_cast<uint32_t>(r) << 16) |
                                    (static_cast<uint32_t>(g) << 8) |
                                    (static_cast<uint32_t>(b) << 0);
                write_pixel_be(x, y, pixel_be);
            }
        }
    }
};

// Convert guest big-endian pixel buffer to host-native ARGB8888 (uint32_t array).
// hostBuf must hold width*height entries. This conversion is fast (word ops).
static void convert_guest_be_to_host_argb(const GuestFramebuffer &fb, uint32_t *hostBuf) {
    // The guest stores pixels as 0xAARRGGBB big-endian (bytes in memory: AA RR GG BB).
    // On little-endian host, we want the uint32 value 0xAARRGGBB in native representation,
    // so we must convert from big-endian word to host endianness.
    const size_t pixels = static_cast<size_t>(fb.width) * static_cast<size_t>(fb.height);
    const uint8_t* src = fb.bytes.data();

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    // Host is little-endian: read big-endian word and swap
    for (size_t i = 0; i < pixels; ++i) {
        uint32_t be =
            (static_cast<uint32_t>(src[i*4 + 0]) << 24) |
            (static_cast<uint32_t>(src[i*4 + 1]) << 16) |
            (static_cast<uint32_t>(src[i*4 + 2]) << 8) |
            (static_cast<uint32_t>(src[i*4 + 3]) << 0);
        hostBuf[i] = SDL_SwapBE32(be); // convert to host order (little)
    }
#else
    // Big-endian host: copy bytes into uint32_t directly
    memcpy(hostBuf, src, pixels * 4);
#endif
}

// Compute destination rectangle preserving aspect ratio (5:4) and optionally integer scale.
static SDL_Rect compute_dest_rect(int winW, int winH, int fbW, int fbH, bool integerScale) {
    float wantedAspect = static_cast<float>(fbW) / static_cast<float>(fbH);
    float winAspect = static_cast<float>(winW) / static_cast<float>(winH);
    SDL_Rect dst{};
    if (!integerScale) {
        if (winAspect > wantedAspect) {
            int h = winH;
            int w = static_cast<int>(h * wantedAspect + 0.5f);
            dst.x = (winW - w) / 2;
            dst.y = 0;
            dst.w = w;
            dst.h = h;
        } else {
            int w = winW;
            int h = static_cast<int>(w / wantedAspect + 0.5f);
            dst.x = 0;
            dst.y = (winH - h) / 2;
            dst.w = w;
            dst.h = h;
        }
    } else {
        // integer scale: find largest integer scale that fits
        int scaleX = winW / fbW;
        int scaleY = winH / fbH;
        int scale = scaleX < scaleY ? scaleX : scaleY;
        if (scale < 1) scale = 1;
        int w = fbW * scale;
        int h = fbH * scale;
        dst.x = (winW - w) / 2;
        dst.y = (winH - h) / 2;
        dst.w = w;
        dst.h = h;
    }
    return dst;
}

int main(int argc, char** argv) {
    std::cout << "Racer framebuffer test (single-file)\n";
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    // Create Guest framebuffer & fill pattern
    GuestFramebuffer fb(FB_W, FB_H, DEFAULT_PITCH);
    fb.fill_test_pattern();

    bool integerScale = false;
    bool useVsync = true;

    // Create window at native size (resizable)
    SDL_Window* window = SDL_CreateWindow("Racer - SGI Octane Framebuffer",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          FB_W, FB_H,
                                          SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 2;
    }

    // Create renderer with or without vsync. We'll recreate if user toggles vsync
    auto create_renderer = [&](bool vsync)->SDL_Renderer* {
        Uint32 flags = SDL_RENDERER_ACCELERATED;
        if (vsync) flags |= SDL_RENDERER_PRESENTVSYNC;
        SDL_Renderer* r = SDL_CreateRenderer(window, -1, flags);
        if (!r) {
            std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
        }
        return r;
    };

    SDL_Renderer* renderer = create_renderer(useVsync);
    if (!renderer) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 3;
    }

    // Create streaming texture - host pixel format ARGB8888 widely supported
    SDL_Texture* texture = SDL_CreateTexture(renderer,
                                             SDL_PIXELFORMAT_ARGB8888,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             FB_W, FB_H);
    if (!texture) {
        std::cerr << "SDL_CreateTexture failed: " << SDL_GetError() << "\n";
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 4;
    }

    // Host conversion buffer
    std::vector<uint32_t> hostBuf;
    hostBuf.resize(static_cast<size_t>(FB_W) * static_cast<size_t>(FB_H));

    std::atomic<bool> running(true);
    auto lastFrame = std::chrono::steady_clock::now();
    const std::chrono::microseconds framePeriod(16667); // ~60Hz

    std::cout << "Controls: ESC=quit  SPACE=refill pattern  i=toggle integer scale  v=toggle vsync\n";

    while (running.load()) {
        // Event handling
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                running.store(false);
            } else if (ev.type == SDL_KEYDOWN) {
                SDL_Keycode k = ev.key.keysym.sym;
                if (k == SDLK_ESCAPE) running.store(false);
                else if (k == SDLK_SPACE) {
                    fb.fill_test_pattern();
                    std::cout << "Refilled test pattern\n";
                } else if (k == SDLK_i) {
                    integerScale = !integerScale;
                    std::cout << "Integer scale: " << (integerScale ? "ON" : "OFF") << "\n";
                } else if (k == SDLK_v) {
                    useVsync = !useVsync;
                    std::cout << "VSync: " << (useVsync ? "ON" : "OFF") << " (recreating renderer)\n";
                    // Recreate renderer and texture
                    if (texture) { SDL_DestroyTexture(texture); texture = nullptr; }
                    if (renderer) { SDL_DestroyRenderer(renderer); renderer = nullptr; }
                    renderer = create_renderer(useVsync);
                    if (!renderer) { running.store(false); break; }
                    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                                SDL_TEXTUREACCESS_STREAMING, FB_W, FB_H);
                    if (!texture) { std::cerr << "CreateTexture failed after vsync toggle\n"; running.store(false); break; }
                }
            } else if (ev.type == SDL_WINDOWEVENT) {
                if (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    // Could respond if needed
                }
            }
        }

        // Convert guest BE framebuffer -> host ARGB8888 array
        convert_guest_be_to_host_argb(fb, hostBuf.data());

        // Update texture
        // We'll update entire texture each frame (could optimize with dirty rects)
        SDL_UpdateTexture(texture, nullptr, hostBuf.data(), FB_W * 4);

        // get window size and destination rect
        int winW, winH;
        SDL_GetWindowSize(window, &winW, &winH);
        SDL_Rect dst = compute_dest_rect(winW, winH, FB_W, FB_H, integerScale);

        // Render
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, &dst);
        SDL_RenderPresent(renderer);

        // Frame cap if vsync is not used
        if (!useVsync) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - lastFrame);
            if (elapsed < framePeriod) {
                std::this_thread::sleep_for(framePeriod - elapsed);
            }
            lastFrame = std::chrono::steady_clock::now();
        }
    }

    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

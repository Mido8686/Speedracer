// Requires linking with SDL2: e.g. -lSDL2
#include <SDL.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <iostream>
#include "dev/framebuffer.h"
#include "memory.h"

using namespace dev;

class SDLFramebufferWindow {
public:
    SDLFramebufferWindow(FramebufferDevice* fbdev)
      : fbdev(fbdev), running(false), window(nullptr), renderer(nullptr), texture(nullptr) {
    }

    ~SDLFramebufferWindow() { stop(); }

    // Start display thread
    void start() {
        running.store(true);
        displayThread = std::thread(&SDLFramebufferWindow::threadMain, this);
    }

    // Stop and join
    void stop() {
        running.store(false);
        if (displayThread.joinable()) displayThread.join();
    }

private:
    FramebufferDevice* fbdev;
    std::atomic<bool> running;
    std::thread displayThread;

    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;

    // conversion buffer if memory mapping not available or if endianness swap needed
    std::vector<uint32_t> convertBuf;

    // choose 60Hz frame period
    const std::chrono::microseconds framePeriod{16667}; // ~1/60s

    void threadMain() {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
            std::cerr << "[SDL] Failed to init SDL: " << SDL_GetError() << "\n";
            return;
        }

        uint32_t fbw = fbdev->get_width();
        uint32_t fbh = fbdev->get_height();

        // Create a window sized to native resolution or scaled up
        // Start window at native resolution but allow resizing
        window = SDL_CreateWindow("Speedracer - SGI Octane Framebuffer",
                                  SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  static_cast<int>(fbw), static_cast<int>(fbh),
                                  SDL_WINDOW_RESIZABLE);
        if (!window) {
            std::cerr << "[SDL] CreateWindow failed: " << SDL_GetError() << "\n";
            SDL_Quit();
            return;
        }

        // Create renderer with vsync ON (present will wait for vsync)
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer) {
            std::cerr << "[SDL] CreateRenderer failed: " << SDL_GetError() << "\n";
            SDL_DestroyWindow(window);
            SDL_Quit();
            return;
        }

        // Create texture of the framebuffer native size.
        // Use ARGB8888 as renderer format; we'll convert if needed.
        texture = SDL_CreateTexture(renderer,
                                    SDL_PIXELFORMAT_ARGB8888,
                                    SDL_TEXTUREACCESS_STREAMING,
                                    static_cast<int>(fbw), static_cast<int>(fbh));
        if (!texture) {
            std::cerr << "[SDL] CreateTexture failed: " << SDL_GetError() << "\n";
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return;
        }

        // Main render loop
        while (running.load()) {
            auto frameStart = std::chrono::steady_clock::now();

            // Poll SDL events (handle window close)
            SDL_Event ev;
            while (SDL_PollEvent(&ev)) {
                if (ev.type == SDL_QUIT) {
                    running.store(false);
                    break;
                } else if (ev.type == SDL_KEYDOWN) {
                    if (ev.key.keysym.sym == SDLK_ESCAPE) running.store(false);
                }
            }
            if (!running.load()) break;

            // If the device indicated a frame is ready (guest wrote VSYNC), present it
            size_t fbsize;
            uint8_t* hostPtr = fbdev->fb_ptr(fbsize);
            bool usedDirectPtr = (hostPtr != nullptr);

            if (usedDirectPtr) {
                // If we have a direct host pointer we can stream to texture fast.
                // But we must ensure pixel format matches ARGB8888: if guest big-endian,
                // swap bytes accordingly. For safety here we'll perform conversion into convertBuf.
                convertBuf.resize(fbw * fbh);
                // For speed, memcpy then possible swap; assume source pixels are 32-bit words in memory.
                uint32_t *src = reinterpret_cast<uint32_t*>(hostPtr);
                // Detect endianness differences
                #if SDL_BYTEORDER == SDL_LIL_ENDIAN
                    // Host is little-endian. Guest is big-endian (likely). Swap bytes.
                    for (size_t i = 0; i < (size_t)(fbw * fbh); ++i) {
                        uint32_t v = src[i];
                        // swap from big-endian uint32_t to host-native
                        uint32_t swapped = SDL_SwapBE32(v);
                        convertBuf[i] = swapped;
                    }
                #else
                    // Host is big-endian; no swap needed
                    memcpy(convertBuf.data(), src, fbw * fbh * sizeof(uint32_t));
                #endif
                // Update texture with convertBuf
                SDL_UpdateTexture(texture, nullptr, convertBuf.data(), fbw * 4);
            } else {
                // No direct pointer: read memory via provided memory API.
                // We'll assume a function memory->read32(phys) exists; if not, you must adapt.
                // As a fallback here we clear with black.
                convertBuf.assign(fbw * fbh, 0xFF000000u);
            }

            // Compute dest rect preserving aspect ratio inside window size
            int winW, winH;
            SDL_GetWindowSize(window, &winW, &winH);
            // desired aspect is 5:4 (1280/1024)
            float wantedAspect = static_cast<float>(fbw) / static_cast<float>(fbh);
            float winAspect = static_cast<float>(winW) / static_cast<float>(winH);
            SDL_Rect dst;
            if (winAspect > wantedAspect) {
                // window is wider -> pillarbox
                int h = winH;
                int w = static_cast<int>(h * wantedAspect + 0.5f);
                dst.x = (winW - w) / 2;
                dst.y = 0;
                dst.w = w;
                dst.h = h;
            } else {
                // window is taller -> letterbox
                int w = winW;
                int h = static_cast<int>(w / wantedAspect + 0.5f);
                dst.x = 0;
                dst.y = (winH - h) / 2;
                dst.w = w;
                dst.h = h;
            }

            // Render
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, nullptr, &dst);
            SDL_RenderPresent(renderer);

            // If guest signalled vsync/write-to-vsync (frameReadyFlag), clear it here
            // Note: our fbdev uses callback; if explicit flag is available you can reset it.
            // Sleep to cap frame rate roughly at 60 Hz (if vsync not already doing it)
            auto frameEnd = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(frameEnd - frameStart);
            if (elapsed < framePeriod) {
                std::this_thread::sleep_for(framePeriod - elapsed);
            }
        }

        // Cleanup
        if (texture) SDL_DestroyTexture(texture);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
    }
};

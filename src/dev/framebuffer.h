// -----------------------------------------------------------
// framebuffer.h
// -----------------------------------------------------------
// Racer SGI Octane1 Emulator
// Simple SI/CRM framebuffer:
//   - 1280x1024
//   - 32-bit ARGB
//   - MMIO mapped via emulator.cpp Part 2
//   - PROM/IRIX will write directly to this buffer
// -----------------------------------------------------------

#pragma once
#include <cstdint>
#include <vector>

class Framebuffer {
public:
    Framebuffer();
    ~Framebuffer();

    // Initialize framebuffer
    void init(uint32_t w, uint32_t h);

    // Access raw pixel buffer
    uint8_t* data() { return pixels.data(); }
    uint32_t size() const { return pixels.size(); }

    // MMIO access (GPU registers)
    uint32_t read_reg(uint32_t offset);
    void     write_reg(uint32_t offset, uint32_t value);

    // Direct screen write (PROM/IRIX)
    void fb_write32(uint32_t offset, uint32_t value);
    uint32_t fb_read32(uint32_t offset);

    // Get dimensions
    uint32_t width()  const { return fb_width;  }
    uint32_t height() const { return fb_height; }

private:
    std::vector<uint8_t> pixels;  // ARGB8888 framebuffer
    uint32_t fb_width  = 1280;
    uint32_t fb_height = 1024;

    // Basic CRM registers (prom expects these)
    uint32_t crm_status  = 0x00000001; // present
    uint32_t crm_boardid = 0x00000020; // SI board ID
};

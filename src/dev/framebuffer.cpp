// -----------------------------------------------------------
// framebuffer.cpp
// -----------------------------------------------------------
// SGI Octane1 SI (CRM) Framebuffer implementation
// -----------------------------------------------------------

#include "framebuffer.h"
#include <cstring>
#include <iostream>

Framebuffer::Framebuffer() {}
Framebuffer::~Framebuffer() {}

void Framebuffer::init(uint32_t w, uint32_t h)
{
    fb_width  = w;
    fb_height = h;

    size_t bytes = (size_t)w * (size_t)h * 4; // ARGB8888
    pixels.resize(bytes);
    std::memset(pixels.data(), 0, bytes);

    std::cout << "[FB] Framebuffer initialized "
              << w << "x" << h << " (" << bytes/1024 << " KB)\n";
}

// -----------------------------------------------------------
// CRM MMIO registers
// -----------------------------------------------------------
uint32_t Framebuffer::read_reg(uint32_t offset)
{
    switch (offset)
    {
    case 0x0000: return crm_status;   // CRM present bit
    case 0x0004: return crm_boardid;  // SI board type
    }

    return 0;
}

void Framebuffer::write_reg(uint32_t offset, uint32_t value)
{
    // Future features:
    // - resolution change
    // - mode switching
    // - cursor registers
    // For now ignore safely.
}

// -----------------------------------------------------------
// Direct framebuffer read/write (PROM writes characters here)
// -----------------------------------------------------------
uint32_t Framebuffer::fb_read32(uint32_t offset)
{
    if (offset + 4 > pixels.size()) return 0;

    return (pixels[offset + 0] << 24) |
           (pixels[offset + 1] << 16) |
           (pixels[offset + 2] <<  8) |
           (pixels[offset + 3] <<  0);
}

void Framebuffer::fb_write32(uint32_t offset, uint32_t value)
{
    if (offset + 4 > pixels.size()) return;

    pixels[offset + 0] = (value >> 24) & 0xFF;
    pixels[offset + 1] = (value >> 16) & 0xFF;
    pixels[offset + 2] = (value >>  8) & 0xFF;
    pixels[offset + 3] = (value >>  0) & 0xFF;
}

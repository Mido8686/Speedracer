#include "dev/framebuffer.h"
#include "memory.h" // your memory abstraction
#include <cstring>
#include <iostream>

using namespace dev;

FramebufferDevice::FramebufferDevice(Memory* mem, uint64_t mmio_base, FrameReadyCb cb)
  : memory(mem), mmioBase(mmio_base), frameCb(cb) {
    // Initialize regs with default Octane screen values
    regs.ctrl = 0;
    regs.width = 1280;
    regs.height = 1024;
    regs.pitch = regs.width * 4; // 4 bytes per pixel
    regs.fb_phys = 0x10000000; // example physical address (you can choose)
    regs.vsync = 0;
    frameReadyFlag.store(false);
}

FramebufferDevice::~FramebufferDevice() {}

uint32_t FramebufferDevice::mmio_read32(uint64_t offset) {
    switch (offset) {
        case 0x00: return regs.ctrl;
        case 0x04: return regs.width;
        case 0x08: return regs.height;
        case 0x0C: return regs.pitch;
        case 0x10: return regs.fb_phys;
        case 0x14: return regs.vsync;
        default:
            std::cerr << "[FBDEV] mmio_read32 unknown offset 0x" << std::hex << offset << std::dec << "\n";
            return 0;
    }
}

void FramebufferDevice::mmio_write32(uint64_t offset, uint32_t val) {
    switch (offset) {
        case 0x00: regs.ctrl = val; break;
        case 0x04: regs.width = val; break;
        case 0x08: regs.height = val; break;
        case 0x0C: regs.pitch = val; break;
        case 0x10: regs.fb_phys = val; break;
        case 0x14:
            regs.vsync = val;
            // guest wrote to vsync register: notify display that a new frame is ready
            frameReadyFlag.store(true);
            if (frameCb) frameCb();
            break;
        default:
            std::cerr << "[FBDEV] mmio_write32 unknown offset 0x" << std::hex << offset << " val=0x" << val << std::dec << "\n";
            break;
    }
}

uint8_t* FramebufferDevice::fb_ptr(size_t& out_size) {
    // We attempt to map the physical memory to a host pointer. If your Memory has a method,
    // use it; otherwise, read via memory->read32 etc.
    // Example assumed API: memory->host_ptr_for_phys(uint32_t phys, size_t size)
    uint64_t phys = static_cast<uint64_t>(regs.fb_phys);
    if (!memory) { out_size = 0; return nullptr; }
    out_size = static_cast<size_t>(regs.pitch) * static_cast<size_t>(regs.height);

    // If Memory exposes direct pointer mapping:
    #ifdef MEMORY_HAS_HOST_POINTER
    uint8_t* p = memory->host_ptr_for_phys(phys, out_size);
    return p;
    #else
    // Fallback: allocate a temporary buffer and keep copying on each frame.
    // The display thread will call this every frame, so we can use memory->readBlock
    // Here we'll return nullptr to indicate fallback is required (display thread should call memory reads).
    return nullptr;
    #endif
}

void FramebufferDevice::set_frame_ready_cb(FrameReadyCb cb) {
    frameCb = cb;
}

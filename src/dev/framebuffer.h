#pragma once
#include <cstdint>
#include <functional>
#include <atomic>

class Memory; // forward

namespace dev {

struct FramebufferRegs {
    uint32_t ctrl;    // bit0 = enable
    uint32_t width;
    uint32_t height;
    uint32_t pitch;   // bytes per scanline
    uint32_t fb_phys; // physical base address of framebuffer
    uint32_t vsync;   // write to trigger vsync/notify
};

class FramebufferDevice {
public:
    // callback to notify display thread that a new frame is available
    using FrameReadyCb = std::function<void()>;

    FramebufferDevice(Memory* mem, uint64_t mmio_base, FrameReadyCb cb = nullptr);
    ~FramebufferDevice();

    // MMIO read/write handlers (call from bus)
    uint32_t mmio_read32(uint64_t offset);
    void mmio_write32(uint64_t offset, uint32_t val);

    // Return pointer to framebuffer memory or nullptr if not available.
    // This is just a convenience for the SDL display to read pixels.
    // size is at least pitch * height bytes.
    uint8_t* fb_ptr(size_t& out_size);

    // Register/replace frame-ready callback
    void set_frame_ready_cb(FrameReadyCb cb);

    // Helpers
    uint32_t get_width() const { return regs.width; }
    uint32_t get_height() const { return regs.height; }
    uint32_t get_pitch() const { return regs.pitch; }

private:
    Memory* memory;
    uint64_t mmioBase;
    FramebufferRegs regs;
    FrameReadyCb frameCb;
    std::atomic<bool> frameReadyFlag;
};

} // namespace dev

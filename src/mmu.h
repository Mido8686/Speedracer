// -----------------------------------------------------------
// mmu.h
// -----------------------------------------------------------
// Racer SGI Octane1 Emulator
// Memory-Management Unit interface
// -----------------------------------------------------------

#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>

// Forward declarations
class Memory;
class CP0;

// Simple TLB entry (placeholder for future full MIPS64)
struct TLBEntry {
    uint64_t vpn2 = 0;
    uint64_t pfn  = 0;
    uint32_t asid = 0;
    bool     valid = false;
    bool     dirty = false;
};

// -----------------------------------------------------------
// MMU class
// -----------------------------------------------------------
class MMU {
public:
    MMU();
    ~MMU();

    void attach_memory(Memory* m);
    void attach_cp0(CP0* c);

    // Flat or TLB-translated memory access
    uint32_t read32(uint64_t vaddr);
    void     write32(uint64_t vaddr, uint32_t value);

    // TLB control
    void reset();
    void set_tlb_enabled(bool en) { enable_tlb = en; }
    bool is_tlb_enabled() const { return enable_tlb; }

private:
    Memory* mem = nullptr;
    CP0*    cp0 = nullptr;
    bool    enable_tlb = false;

    // Simple 64-entry table for later full implementation
    static constexpr int MAX_TLB = 64;
    TLBEntry tlb[MAX_TLB];

    uint64_t translate(uint64_t vaddr);
    uint64_t tlb_translate(uint64_t vaddr);
};

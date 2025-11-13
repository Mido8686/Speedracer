// -----------------------------------------------------------
// mmu.cpp (Part 1)
// -----------------------------------------------------------
// Speedracer SGI Octane1 Emulator
// Flat + stubbed TLB MMU implementation
// -----------------------------------------------------------

#include "mmu.h"
#include "memory.h"
#include "cp0.h"
#include <iostream>

// -----------------------------------------------------------
// Constructor / Destructor
// -----------------------------------------------------------
MMU::MMU() = default;
MMU::~MMU() = default;

// -----------------------------------------------------------
// attach subsystems
// -----------------------------------------------------------
void MMU::attach_memory(Memory* m) { mem = m; }
void MMU::attach_cp0(CP0* c)       { cp0 = c; }

// -----------------------------------------------------------
// reset
// -----------------------------------------------------------
void MMU::reset() {
    enable_tlb = false;
    for (auto& e : tlb) e.valid = false;
    std::cout << "[MMU] Reset — flat mode enabled\n";
}

// -----------------------------------------------------------
// translate – map virtual → physical
// -----------------------------------------------------------
uint64_t MMU::translate(uint64_t vaddr) {
    if (!enable_tlb) {
        // Flat mapping: strip upper bits (KSEG0/KSEG1)
        return vaddr & 0x1FFFFFFF;
    }
    return tlb_translate(vaddr);
}

// -----------------------------------------------------------
// tlb_translate – stubbed TLB lookup
// -----------------------------------------------------------
uint64_t MMU::tlb_translate(uint64_t vaddr) {
    for (const auto& e : tlb) {
        if (e.valid && ((vaddr >> 12) == e.vpn2))
            return (e.pfn << 12) | (vaddr & 0xFFF);
    }
    throw std::runtime_error("[MMU] TLB miss at 0x" + std::to_string(vaddr));
}

// -----------------------------------------------------------
// read32 / write32
// -----------------------------------------------------------
uint32_t MMU::read32(uint64_t vaddr) {
    if (!mem) throw std::runtime_error("[MMU] Memory not attached");
    uint64_t phys = translate(vaddr);
    return mem->read32(phys);
}

void MMU::write32(uint64_t vaddr, uint32_t value) {
    if (!mem) throw std::runtime_error("[MMU] Memory not attached");
    uint64_t phys = translate(vaddr);
    mem->write32(phys, value);
}

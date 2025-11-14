// -----------------------------------------------------------
// memory.cpp
// -----------------------------------------------------------
// Speedracer SGI Octane1 Emulator
// RAM implementation with safe read/write
// -----------------------------------------------------------

#include "memory.h"
#include <iostream>

// -----------------------------------------------------------
// Constructor / Destructor
// -----------------------------------------------------------
Memory::Memory() {}
Memory::~Memory() {}

// -----------------------------------------------------------
// init() - allocate RAM
// -----------------------------------------------------------
void Memory::init(uint64_t size_bytes) {
    ram.resize(size_bytes);
    std::memset(ram.data(), 0, ram.size());

    std::cout << "[MEM] RAM initialized: " << size_bytes / (1024*1024)
              << " MB\n";
}

// -----------------------------------------------------------
// read helpers
// -----------------------------------------------------------
uint8_t Memory::read8(uint64_t phys) {
    check_bounds(phys, 1);
    return ram[phys];
}

uint16_t Memory::read16(uint64_t phys) {
    check_bounds(phys, 2);
    return (ram[phys] << 8) | ram[phys + 1];
}

uint32_t Memory::read32(uint64_t phys) {
    check_bounds(phys, 4);
    return (ram[phys] << 24) |
           (ram[phys + 1] << 16) |
           (ram[phys + 2] << 8) |
            ram[phys + 3];
}

uint64_t Memory::read64(uint64_t phys) {
    check_bounds(phys, 8);
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) {
        v = (v << 8) | ram[phys + i];
    }
    return v;
}

// -----------------------------------------------------------
// write helpers
// -----------------------------------------------------------
void Memory::write8(uint64_t phys, uint8_t v) {
    check_bounds(phys, 1);
    ram[phys] = v;
}

void Memory::write16(uint64_t phys, uint16_t v) {
    check_bounds(phys, 2);
    ram[phys]     = (v >> 8) & 0xFF;
    ram[phys + 1] = v & 0xFF;
}

void Memory::write32(uint64_t phys, uint32_t v) {
    check_bounds(phys, 4);
    ram[phys]     = (v >> 24) & 0xFF;
    ram[phys + 1] = (v >> 16) & 0xFF;
    ram[phys + 2] = (v >> 8)  & 0xFF;
    ram[phys + 3] = v & 0xFF;
}

void Memory::write64(uint64_t phys, uint64_t v) {
    check_bounds(phys, 8);
    for (int i = 7; i >= 0; i--) {
        ram[phys + (7 - i)] = (v >> (i * 8)) & 0xFF;
    }
}

// -----------------------------------------------------------
// load_blob()
// -----------------------------------------------------------
void Memory::load_blob(uint64_t phys, const void* data, size_t size) {
    check_bounds(phys, size);
    const uint8_t* p = reinterpret_cast<const uint8_t*>(data);
    std::memcpy(&ram[phys], p, size);
}

// -----------------------------------------------------------
// clear_region()
// -----------------------------------------------------------
void Memory::clear_region(uint64_t phys, uint64_t size) {
    check_bounds(phys, size);
    std::memset(&ram[phys], 0, size);
}

// -----------------------------------------------------------
// emulator.cpp (Part 1 — Core System Setup)
// -----------------------------------------------------------
// Speedracer SGI Octane1 Emulator
// Core initialization:
//   - Create CPU, MMU, CP0, Memory
//   - Wire all subsystems together
//   - Provide run loop
//   - Provide physical read/write for devices
// -----------------------------------------------------------

#include "emulator.h"
#include "cpu.h"
#include "mmu.h"
#include "memory.h"
#include "cp0.h"
#include <iostream>

Emulator::Emulator()
{
    cpu  = new CPU();
    mmu  = new MMU();
    cp0  = new CP0();
    mem  = new Memory();
}

Emulator::~Emulator()
{
    delete cpu;
    delete mmu;
    delete cp0;
    delete mem;
}

// -----------------------------------------------------------
// Initialize emulator
// -----------------------------------------------------------
bool Emulator::init(uint64_t ram_size)
{
    std::cout << "[Emu] Initializing system...\n";

    // Setup RAM
    mem->init(ram_size);

    // Attach subsystems
    cpu->attach_mmu(mmu);
    cpu->attach_cp0(cp0);

    mmu->attach_memory(mem);
    mmu->attach_cp0(cp0);

    cp0->attach_cpu(cpu);

    // Reset all components
    cp0->reset();
    mmu->reset();
    cpu->reset();

    std::cout << "[Emu] System ready.\n";
    return true;
}

// -----------------------------------------------------------
// Load PROM (declared in Part 2)
// -----------------------------------------------------------
bool Emulator::load_prom(const std::string& path);

// -----------------------------------------------------------
// Main execution loop
// -----------------------------------------------------------
void Emulator::run(uint64_t cycles)
{
    std::cout << "[Emu] Starting CPU...\n";

    for (uint64_t i = 0; i < cycles; i++)
    {
        cpu->step();

        // For future: handle timers, interrupts, VBLANK, DMA
    }
}

// -----------------------------------------------------------
// System read/write (to be filled by Part 2: MMIO)
// Default behavior: direct RAM access
// -----------------------------------------------------------
uint32_t Emulator::sys_read32(uint64_t phys)
{
    return mem->read32(phys);
}

void Emulator::sys_write32(uint64_t phys, uint32_t val)
{
    mem->write32(phys, val);
}

// -----------------------------------------------------------
// emulator.cpp  (Part 2 — Octane1 SI Hardware Map)
// -----------------------------------------------------------
// This section implements MMIO (HEART, HUB, MACE, CRM Graphics)
// for SGI Octane1 PROM boot compatibility.
// -----------------------------------------------------------
// -----------------------------------------------------------
// Octane1 IP30 Memory Map (subset required for PROM 4.9)
// -----------------------------------------------------------
// Physical ranges (PROM probes these):
//
//   0x1F000000 - HEART registers
//   0x1F200000 - HUB registers
//   0x1F400000 - MACE (I/O + UART)
//   0x1F600000 - CRM / SI Graphics
//
// Each region is 2MB.
// -----------------------------------------------------------

static constexpr uint64_t HEART_BASE = 0x1F000000;
static constexpr uint64_t HUB_BASE   = 0x1F200000;
static constexpr uint64_t MACE_BASE  = 0x1F400000;
static constexpr uint64_t CRM_BASE   = 0x1F600000;

static constexpr uint64_t MMIO_SIZE  = 0x00200000; // 2MB per region

// -----------------------------------------------------------
// MMIO READ32
// -----------------------------------------------------------
uint32_t Emulator::mmio_read32(uint64_t phys)
{
    // -----------------------------
    // HEART
    // -----------------------------
    if (phys >= HEART_BASE && phys < HEART_BASE + MMIO_SIZE)
    {
        uint32_t off = phys - HEART_BASE;

        // PROM checks this register to know the chip exists.
        if (off == 0x0000)
            return 0x00010001;  // fake HEART version

        return 0;
    }

    // -----------------------------
    // HUB
    // -----------------------------
    if (phys >= HUB_BASE && phys < HUB_BASE + MMIO_SIZE)
    {
        uint32_t off = phys - HUB_BASE;

        // PROM checks number of RAM banks
        if (off == 0x0010)
            return 0x2; // pretend 2 banks present

        return 0;
    }

    // -----------------------------
    // MACE (PROM UART + misc I/O)
    // -----------------------------
    if (phys >= MACE_BASE && phys < MACE_BASE + MMIO_SIZE)
    {
        uint32_t off = phys - MACE_BASE;

        // PROM polls this for console input
        // No input → return -1
        if (off == 0x50000)
            return 0xFFFFFFFF;

        return 0;
    }

    // -----------------------------
    // CRM / SI Graphics
    // -----------------------------
    if (phys >= CRM_BASE && phys < CRM_BASE + MMIO_SIZE)
    {
        uint32_t off = phys - CRM_BASE;

        // PROM checks CRM "present" bit
        if (off == 0x0000)
            return 0x00000001;

        // Board type: 0x20 = SI Graphics
        if (off == 0x0004)
            return 0x00000020;

        return 0;
    }

    // -----------------------------
    // Unknown MMIO
    // -----------------------------
    std::cerr << "[MMIO] Unknown read32 @ 0x"
              << std::hex << phys << std::dec << "\n";

    return 0;
}

// -----------------------------------------------------------
// MMIO WRITE32
// -----------------------------------------------------------
void Emulator::mmio_write32(uint64_t phys, uint32_t val)
{
    // -----------------------------
    // HEART
    // -----------------------------
    if (phys >= HEART_BASE && phys < HEART_BASE + MMIO_SIZE)
    {
        // HEART writes are ignored safely
        return;
    }

    // -----------------------------
    // HUB
    // -----------------------------
    if (phys >= HUB_BASE && phys < HUB_BASE + MMIO_SIZE)
    {
        // HUB writes ignored
        return;
    }

    // -----------------------------
    // MACE — PROM serial output
    // -----------------------------
    if (phys >= MACE_BASE && phys < MACE_BASE + MMIO_SIZE)
    {
        uint32_t off = phys - MACE_BASE;

        // PROM writes serial output characters here:
        if (off == 0x50000)
        {
            char c = (char)(val & 0xFF);
            std::cout << c; // print PROM output
            return;
        }

        return;
    }

    // -----------------------------
    // CRM / SI Graphics
    // -----------------------------
    if (phys >= CRM_BASE && phys < CRM_BASE + MMIO_SIZE)
    {
        // For now, ignore writes — framebuffer handled in Part 3
        return;
    }

    // -----------------------------
    // Unknown MMIO
    // -----------------------------
    std::cerr << "[MMIO] Unknown write32 @ 0x"
              << std::hex << phys
              << " = 0x" << val
              << std::dec << "\n";
}

// -----------------------------------------------------------
// System read/write entry points (CPU calls these)
// -----------------------------------------------------------
uint32_t Emulator::sys_read32(uint64_t phys)
{
    if (phys >= HEART_BASE)
        return mmio_read32(phys);

    return mem->read32(phys);
}

void Emulator::sys_write32(uint64_t phys, uint32_t val)
{
    if (phys >= HEART_BASE)
        mmio_write32(phys, val);
    else
        mem->write32(phys, val);
}

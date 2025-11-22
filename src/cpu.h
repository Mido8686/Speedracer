// -----------------------------------------------------------
// cpu.h
// -----------------------------------------------------------
// Racer SGI Octane1 Emulator
// MIPS CPU Core - Header Definition
//
// Provides the CPU class interface used by cpu.cpp
// -----------------------------------------------------------

#pragma once
#include <cstdint>
#include <iostream>

// Forward declarations to avoid circular includes
class MMU;
class CP0;
class Memory;

// -----------------------------------------------------------
// CPU class
// -----------------------------------------------------------
class CPU {
public:
    CPU();
    ~CPU();

    // Core control
    void reset();
    void stepOnce_mmu();

    // Attach subsystems
    void attach_mmu(MMU *m);
    void attach_cp0(CP0 *c);
    void attach_memory(Memory *m);

    // Register access
    uint64_t read_reg(uint32_t idx) const;
    void write_reg(uint32_t idx, uint64_t val);

    // Program counter control
    void setPC(uint64_t addr);
    uint64_t getPC() const { return pc; }

    // Execution state
    bool is_halted() const;
    void halt() { halted = true; }

private:
    // General-purpose registers (MIPS64 has 32)
    uint64_t regs[32];

    // Special registers
    uint64_t hi = 0;
    uint64_t lo = 0;

    // Program counters
    uint64_t pc = 0;
    uint64_t nextPC = 0;

    // Halt flag
    bool halted = false;

    // Connections to subsystems
    MMU *mmu = nullptr;
    CP0 *cp0 = nullptr;
    Memory *mem = nullptr;
};

// -----------------------------------------------------------
// cpu.cpp (Fixed)
// -----------------------------------------------------------
// Speedracer SGI Octane1 Emulator
// MIPS CPU Core (Interpreter Mode)
//
// Cleaned and compile-ready version.
// -----------------------------------------------------------

#include "cpu.h"
#include "mmu.h"
#include "cp0.h"
#include "memory.h"
#include <iostream>
#include <cstring>
#include <stdexcept>

// -----------------------------------------------------------
// Constructor / Destructor
// -----------------------------------------------------------

CPU::CPU() {
    reset();
}

CPU::~CPU() = default;

// -----------------------------------------------------------
// reset() - Reset CPU state
// -----------------------------------------------------------

void CPU::reset() {
    std::memset(regs, 0, sizeof(regs));
    pc = 0x1FC00000;     // PROM entry point
    nextPC = pc + 4;
    hi = lo = 0;
    halted = false;

    std::cout << "[CPU] Reset complete. PC=0x" << std::hex << pc << std::dec << "\n";
}

// -----------------------------------------------------------
// attach_* methods
// -----------------------------------------------------------

void CPU::attach_mmu(MMU *m) { mmu = m; }
void CPU::attach_cp0(CP0 *c) { cp0 = c; }
void CPU::attach_memory(Memory *m) { mem = m; }

// -----------------------------------------------------------
// stepOnce_mmu() - Execute one instruction (MMU-aware)
// -----------------------------------------------------------

void CPU::stepOnce_mmu() {
    if (halted) return;

    uint32_t instr = 0;
    try {
        instr = mmu->read32(pc);
    } catch (...) {
        std::cerr << "[CPU] Fetch fault at PC=0x" << std::hex << pc << std::dec << "\n";
        halted = true;
        return;
    }

    uint32_t opcode = instr >> 26;

    // Minimal MIPS interpreter (placeholder)
    switch (opcode) {
        case 0x00: { // SPECIAL
            uint32_t funct = instr & 0x3F;
            switch (funct) {
                case 0x0D: // BREAK
                    std::cout << "[CPU] BREAK at 0x" << std::hex << pc << std::dec << "\n";
                    halted = true;
                    break;
                default:
                    std::cerr << "[CPU] Unimplemented funct=0x" << std::hex << funct << std::dec << "\n";
                    halted = true;
                    break;
            }
            break;
        }

        case 0x02: { // J
            uint32_t target = (instr & 0x03FFFFFF) << 2;
            nextPC = (pc & 0xF0000000) | target;
            break;
        }

        case 0x03: { // JAL
            regs[31] = pc + 8;
            uint32_t target = (instr & 0x03FFFFFF) << 2;
            nextPC = (pc & 0xF0000000) | target;
            break;
        }

        default:
            std::cerr << "[CPU] Unimplemented opcode=0x" << std::hex << opcode
                      << " at PC=0x" << pc << std::dec << "\n";
            halted = true;
            break;
    }

    pc = nextPC;
    nextPC += 4;
}

// -----------------------------------------------------------
// Utility helpers
// -----------------------------------------------------------

void CPU::setPC(uint64_t addr) {
    pc = addr;
    nextPC = pc + 4;
}

bool CPU::is_halted() const { return halted; }

uint64_t CPU::read_reg(uint32_t idx) const {
    if (idx < 32) return regs[idx];
    return 0;
}

void CPU::write_reg(uint32_t idx, uint64_t val) {
    if (idx < 32) regs[idx] = val;
}

// -----------------------------------------------------------
// END OF FILE
// -----------------------------------------------------------

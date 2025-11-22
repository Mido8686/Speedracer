// -----------------------------------------------------------
// cp0.h
// -----------------------------------------------------------
// Racer SGI Octane1 Emulator
// MIPS CP0 (System Control Coprocessor)
// Minimal implementation to support:
//  - Status register
//  - Cause register
//  - EPC (exception PC)
//  - BadVAddr
//  - TLB enable flag (MMU integration)
//
// This file is safe, clean, and compiles with all other parts.
// -----------------------------------------------------------

#pragma once
#include <cstdint>
#include <iostream>

// Forward declaration
class CPU;

class CP0 {
public:
    CP0();
    ~CP0();

    void reset();

    // Attach CPU for exception handling
    void attach_cpu(CPU* c);

    // Register access
    uint64_t read_reg(uint32_t idx) const;
    void     write_reg(uint32_t idx, uint64_t value);

    // Exception handling (minimal)
    void raise_exception(uint32_t code, uint64_t badaddr);

    // State queries
    bool is_tlb_enabled() const;

private:
    CPU* cpu = nullptr;

    // CP0 registers (simplified)
    uint64_t index     = 0;    // 0
    uint64_t random    = 0;    // 1
    uint64_t entry_lo0 = 0;    // 2
    uint64_t entry_lo1 = 0;    // 3
    uint64_t context   = 0;    // 4
    uint64_t pagemask  = 0;    // 5
    uint64_t wired     = 0;    // 6
    uint64_t bad_vaddr = 0;    // 8
    uint64_t count     = 0;    // 9
    uint64_t entry_hi  = 0;    // 10
    uint64_t compare   = 0;    // 11
    uint64_t status    = 0;    // 12
    uint64_t cause     = 0;    // 13
    uint64_t epc       = 0;    // 14
    uint64_t prid      = 0;    // 15

    // TLB enable flag (bit in Status)
    // In MIPS: Status bit 31 = ERL, bit 1 = EXL, bit 0 = IE
    // MMU full TLB translation is active when EXL=0 && ERL=0
};

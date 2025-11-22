// -----------------------------------------------------------
// cp0.cpp
// -----------------------------------------------------------
// Racer SGI Octane1 Emulator
// Minimal CP0 implementation
// -----------------------------------------------------------

#include "cp0.h"
#include "cpu.h"

CP0::CP0() {}
CP0::~CP0() {}

void CP0::reset() {
    index = random = entry_lo0 = entry_lo1 = 0;
    context = pagemask = wired = 0;
    bad_vaddr = 0;
    count = compare = 0;
    entry_hi = 0;

    status = 0x00000000;   // interrupt disabled, kernel mode
    cause  = 0x00000000;
    epc    = 0xFFFFFFFFFFFFFFFF;

    prid = 0x00000000;     // placeholder (R10000 PRID later)

    std::cout << "[CP0] Reset complete.\n";
}

void CP0::attach_cpu(CPU* c) {
    cpu = c;
}

// -----------------------------------------------------------
// read_reg() - simplified CP0 read
// -----------------------------------------------------------
uint64_t CP0::read_reg(uint32_t idx) const {
    switch (idx) {
        case 0:  return index;
        case 1:  return random;
        case 2:  return entry_lo0;
        case 3:  return entry_lo1;
        case 4:  return context;
        case 5:  return pagemask;
        case 6:  return wired;
        case 8:  return bad_vaddr;
        case 9:  return count;
        case 10: return entry_hi;
        case 11: return compare;
        case 12: return status;
        case 13: return cause;
        case 14: return epc;
        case 15: return prid;
        default:
            std::cerr << "[CP0] Warning: unimplemented register read " << idx << "\n";
            return 0;
    }
}

// -----------------------------------------------------------
// write_reg() - simplified CP0 write
// -----------------------------------------------------------
void CP0::write_reg(uint32_t idx, uint64_t value) {
    switch (idx) {
        case 2:  entry_lo0 = value; break;
        case 3:  entry_lo1 = value; break;
        case 4:  context   = value; break;
        case 5:  pagemask  = value; break;
        case 6:  wired     = value; break;
        case 9:  count     = value; break;
        case 10: entry_hi  = value; break;
        case 11: compare   = value; break;
        case 12: status    = value; break;
        case 13: cause     = value; break;
        case 14: epc       = value; break;
        default:
            std::cerr << "[CP0] Warning: unimplemented register write " 
                      << idx << " = 0x" << std::hex << value << std::dec << "\n";
            break;
    }
}

// -----------------------------------------------------------
// raise_exception()
// -----------------------------------------------------------
void CP0::raise_exception(uint32_t code, uint64_t badaddr) {
    bad_vaddr = badaddr;
    cause = (cause & ~0x7C) | (code << 2); // simple encoding

    // Save PC where exception occurred
    if (cpu) epc = cpu->getPC();

    // Set EXL bit (bit 1)
    status |= (1 << 1);

    std::cerr << "[CP0] Exception " << code 
              << " at PC=0x" << std::hex << epc 
              << " badaddr=0x" << badaddr << std::dec << "\n";
}

// -----------------------------------------------------------
// is_tlb_enabled()
// -----------------------------------------------------------
bool CP0::is_tlb_enabled() const {
    // MMU full TLB mode active only if:
    //  EXL=0 (bit 1), ERL=0 (bit 2)
    bool exl = (status >> 1) & 1;
    bool erl = (status >> 2) & 1;
    return (!exl && !erl);
}

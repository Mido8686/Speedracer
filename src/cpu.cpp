// speedracer/src/cpu.cpp
#include "cpu.h"
#include <iostream>
#include <iomanip>

MIPSCpu::MIPSCpu(MemoryBus &b) : bus(b) {
    reset(0xBFC00000u); // default PC to PROM base (but caller may set)
}

void MIPSCpu::reset(uint32_t entry_pc) {
    GPR.fill(0);
    PC = entry_pc;
    running = true;
    // convention: register $zero is always 0
    GPR[0] = 0;
}

uint32_t MIPSCpu::fetch32(uint32_t addr) {
    return bus.read32(addr);
}

void MIPSCpu::run(uint64_t max_instructions) {
    uint64_t executed = 0;
    while (running && executed < max_instructions) {
        step();
        ++executed;
    }
    if (executed >= max_instructions) {
        std::cout << "[CPU] Stopped after reaching max instruction count (" << max_instructions << ")\n";
    } else {
        std::cout << "[CPU] Halted after " << executed << " instructions\n";
    }
}

void MIPSCpu::step() {
    uint32_t instr = fetch32(PC);
    // For safety: if fetch returns 0xFFFFFFFF (unmapped), stop
    if (instr == 0xffffffffu) {
        std::cout << "[CPU] Fetch from unmapped memory at PC=0x" << std::hex << PC << std::dec << " — halting\n";
        running = false;
        return;
    }
    // Debug trace: print PC and instruction
    std::cout << "[CPU] PC=0x" << std::hex << PC << " instr=0x" << instr << std::dec << "\n";

    // Advance PC (MIPS semantics: PC holds current instruction; next PC is PC+4 unless branch/jump modifies)
    uint32_t next_pc = PC + 4;

    // decode & execute
    decode_execute(instr);

    // if instruction didn't change PC (jr/j/jal set PC inside), update it to next_pc
    if (PC == (next_pc - 4)) {
        // decode_execute hasn't changed PC (we detect by checking if PC remains same as old PC),
        // so set PC := next_pc. Simpler: ensure PC is updated to next_pc unless modified.
        PC = next_pc;
    }
}

void MIPSCpu::decode_execute(uint32_t instr) {
    uint32_t opcode = (instr >> 26) & 0x3F;
    switch (opcode) {
        case 0x00: // R-type (functions)
            op_rtype(instr);
            break;
        case 0x0f: // LUI
            op_lui(instr);
            break;
        case 0x08: // ADDI
            op_addi(instr);
            break;
        case 0x02: // J
            op_j(instr);
            break;
        case 0x03: // JAL
            op_jal(instr);
            break;
        default:
            std::cout << "[CPU] Unhandled opcode 0x" << std::hex << opcode << std::dec << " — treating as NOP\n";
            // treat as NOP for now
            break;
    }
}

// Implement LUI: opcode 0x0F
void MIPSCpu::op_lui(uint32_t instr) {
    uint32_t rt = (instr >> 16) & 0x1F;
    uint32_t imm = instr & 0xFFFF;
    uint32_t val = imm << 16;
    GPR[rt] = val;
    std::cout << "[CPU] LUI $r" << rt << " <- 0x" << std::hex << val << std::dec << "\n";
    // PC update handled by caller
}

// Implement ADDI: opcode 0x08 (signed immediate)
void MIPSCpu::op_addi(uint32_t instr) {
    uint32_t rs = (instr >> 21) & 0x1F;
    uint32_t rt = (instr >> 16) & 0x1F;
    int32_t imm = static_cast<int16_t>(instr & 0xFFFF);
    uint32_t res = static_cast<uint32_t>(static_cast<int64_t>(GPR[rs]) + imm);
    GPR[rt] = res;
    std::cout << "[CPU] ADDI $r" << rt << " = $r" << rs << " + " << imm << " -> 0x" << std::hex << res << std::dec << "\n";
}

// Implement J (opcode 0x02)
void MIPSCpu::op_j(uint32_t instr) {
    uint32_t target = instr & 0x03FFFFFF;
    // new PC = (PC & 0xF0000000) | (target << 2)
    uint32_t newpc = (PC & 0xF0000000u) | (target << 2);
    std::cout << "[CPU] J to 0x" << std::hex << newpc << std::dec << "\n";
    PC = newpc;
}

// Implement JAL (opcode 0x03)
void MIPSCpu::op_jal(uint32_t instr) {
    uint32_t target = instr & 0x03FFFFFF;
    uint32_t link = PC + 8; // link = address of instruction after delay slot (MIPS)
    GPR[31] = link;
    uint32_t newpc = (PC & 0xF0000000u) | (target << 2);
    std::cout << "[CPU] JAL to 0x" << std::hex << newpc << ", $ra=0x" << link << std::dec << "\n";
    PC = newpc;
}

// Handle R-type functions (e.g., jr)
void MIPSCpu::op_rtype(uint32_t instr) {
    uint32_t rs = (instr >> 21) & 0x1F;
    uint32_t rt = (instr >> 16) & 0x1F;
    uint32_t rd = (instr >> 11) & 0x1F;
    uint32_t shamt = (instr >> 6) & 0x1F;
    uint32_t funct = instr & 0x3F;

    switch (funct) {
        case 0x08: { // JR
            uint32_t target = static_cast<uint32_t>(GPR[rs] & 0xFFFFFFFFu);
            std::cout << "[CPU] JR $r" << rs << " -> 0x" << std::hex << target << std::dec << "\n";
            PC = target;
            break;
        }
        case 0x09: { // JALR (jalr rd, rs) - save PC+8 to rd, jump to rs
            uint32_t link = PC + 8;
            uint32_t target = static_cast<uint32_t>(GPR[rs] & 0xFFFFFFFFu);
            uint32_t dest = rd ? rd : 31;
            GPR[dest] = link;
            PC = target;
            std::cout << "[CPU] JALR r" << dest << " <- 0x" << std::hex << link << ", PC <- 0x" << target << std::dec << "\n";
            break;
        }
        default:
            std::cout << "[CPU] Unhandled R-type funct 0x" << std::hex << funct << std::dec << " — treat as NOP\n";
            break;
    }
}

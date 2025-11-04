#include <iostream>
#include <cstdint>
#include <iomanip>
#include <string>
#include "memory.hpp" // later we'll make this header from memory.cpp

// -----------------------------------------------------------
// Speedracer Emulator - SGI Octane1 (IP30)
// CPU Core (MIPS R10000 Skeleton)
// -----------------------------------------------------------
// Author: Mohamed Zeidan
// Date: November 2025
// -----------------------------------------------------------

class CPU {
public:
    CPU(Memory& mem) : memory(mem) {
        reset();
    }

    void reset() {
        PC = 0xBFC00000; // MIPS reset vector
        for (auto &r : GPR) r = 0;
        HI = LO = 0;
        std::cout << "CPU Reset -> PC = 0x" << std::hex << PC << std::dec << "\n";
    }

    // Single-step execute cycle
    void step() {
        uint32_t instr = memory.read32(PC);
        std::cout << "[FETCH] PC=0x" << std::hex << PC
                  << " INSTR=0x" << instr << std::dec << "\n";
        decodeExecute(instr);
        PC += 4;
    }

private:
    uint64_t GPR[32]; // General Purpose Registers
    uint64_t HI, LO;  // Multiply/Divide registers
    uint64_t PC;      // Program Counter
    Memory& memory;

    void decodeExecute(uint32_t instr) {
        uint32_t opcode = (instr >> 26) & 0x3F;
        switch (opcode) {
            case 0x00: // SPECIAL
                handleSPECIAL(instr);
                break;
            case 0x02: // J
                handleJ(instr);
                break;
            case 0x03: // JAL
                handleJAL(instr);
                break;
            default:
                std::cout << "[WARN] Unimplemented opcode: 0x"
                          << std::hex << opcode << std::dec << "\n";
                break;
        }
    }

    void handleSPECIAL(uint32_t instr);
    void handleJ(uint32_t instr);
    void handleJAL(uint32_t instr);
};

// -----------------------------------------------------------
// CPU Core - Instruction Handlers (Part 2)
// -----------------------------------------------------------

void CPU::handleSPECIAL(uint32_t instr) {
    uint32_t funct = instr & 0x3F;
    uint32_t rs = (instr >> 21) & 0x1F;
    uint32_t rt = (instr >> 16) & 0x1F;
    uint32_t rd = (instr >> 11) & 0x1F;
    uint32_t shamt = (instr >> 6) & 0x1F;

    switch (funct) {
        case 0x20: // ADD
            GPR[rd] = static_cast<int32_t>(GPR[rs]) + static_cast<int32_t>(GPR[rt]);
            std::cout << "[EXEC] ADD $" << rd << ", $" << rs << ", $" << rt << "\n";
            break;

        case 0x21: // ADDU
            GPR[rd] = GPR[rs] + GPR[rt];
            std::cout << "[EXEC] ADDU $" << rd << ", $" << rs << ", $" << rt << "\n";
            break;

        case 0x22: // SUB
            GPR[rd] = static_cast<int32_t>(GPR[rs]) - static_cast<int32_t>(GPR[rt]);
            std::cout << "[EXEC] SUB $" << rd << ", $" << rs << ", $" << rt << "\n";
            break;

        case 0x25: // OR
            GPR[rd] = GPR[rs] | GPR[rt];
            std::cout << "[EXEC] OR $" << rd << ", $" << rs << ", $" << rt << "\n";
            break;

        case 0x24: // AND
            GPR[rd] = GPR[rs] & GPR[rt];
            std::cout << "[EXEC] AND $" << rd << ", $" << rs << ", $" << rt << "\n";
            break;

        case 0x08: // JR
            std::cout << "[EXEC] JR $" << rs << " -> Jump to 0x" << std::hex << GPR[rs] << std::dec << "\n";
            PC = GPR[rs];
            break;

        default:
            std::cout << "[WARN] Unhandled SPECIAL funct=0x"
                      << std::hex << funct << std::dec << "\n";
            break;
    }
}

void CPU::handleJ(uint32_t instr) {
    uint32_t target = (instr & 0x03FFFFFF) << 2;
    uint32_t newPC = (PC & 0xF0000000) | target;
    std::cout << "[EXEC] J -> Jump to 0x" << std::hex << newPC << std::dec << "\n";
    PC = newPC - 4; // adjust since step() adds +4
}

void CPU::handleJAL(uint32_t instr) {
    uint32_t target = (instr & 0x03FFFFFF) << 2;
    GPR[31] = PC + 8; // link
    uint32_t newPC = (PC & 0xF0000000) | target;
    std::cout << "[EXEC] JAL -> Jump to 0x" << std::hex << newPC
              << ", link=$31=0x" << (PC + 8) << std::dec << "\n";
    PC = newPC - 4;
}

// Utility: print registers for debug
void dumpRegisters(const CPU& cpu) {
    std::cout << "\n--- Register Dump ---\n";
    for (int i = 0; i < 32; ++i) {
        std::cout << "R" << std::setw(2) << std::setfill('0') << i << ": "
                  << "0x" << std::hex << cpu.getReg(i) << std::dec << "  ";
        if ((i + 1) % 4 == 0) std::cout << "\n";
    }
    std::cout << "HI: 0x" << std::hex << cpu.getHI()
              << "  LO: 0x" << cpu.getLO() << std::dec << "\n";
}

// -----------------------------------------------------------
// CPU Core - Accessors and Run Loop (Part 3)
// -----------------------------------------------------------

uint64_t CPU::getReg(int index) const {
    if (index < 0 || index >= 32) return 0;
    return GPR[index];
}

uint64_t CPU::getHI() const { return HI; }
uint64_t CPU::getLO() const { return LO; }

void CPU::setReg(int index, uint64_t value) {
    if (index == 0) return; // $zero is always 0
    if (index < 0 || index >= 32) return;
    GPR[index] = value;
}

void CPU::setHI(uint64_t value) { HI = value; }
void CPU::setLO(uint64_t value) { LO = value; }

void CPU::run(int cycles) {
    std::cout << "\n[CPU] Starting execution for " << cycles << " cycles...\n";
    for (int i = 0; i < cycles; ++i) {
        step();
    }
    std::cout << "[CPU] Execution stopped.\n";
}

// -----------------------------------------------------------
// Basic Self-Test Harness (for Debug)
// -----------------------------------------------------------
#ifdef CPU_SELFTEST
int main() {
    Memory mem(1024 * 1024);
    CPU cpu(mem);
    cpu.reset();

    // Example dummy instruction area (filled with zeros)
    cpu.run(10);
    dumpRegisters(cpu);
    return 0;
}
#endif

// -----------------------------------------------------------
// CPU Core - Extended Opcodes (Part 4)
// -----------------------------------------------------------

void CPU::decodeExecute(uint32_t instr) {
    uint32_t opcode = (instr >> 26) & 0x3F;
    uint32_t rs = (instr >> 21) & 0x1F;
    uint32_t rt = (instr >> 16) & 0x1F;
    uint32_t rd = (instr >> 11) & 0x1F;
    uint32_t imm = instr & 0xFFFF;

    switch (opcode) {
        case 0x00: // SPECIAL
            handleSPECIAL(instr);
            break;

        case 0x02: // J
            handleJ(instr);
            break;

        case 0x03: // JAL
            handleJAL(instr);
            break;

        case 0x08: // ADDI
            GPR[rt] = static_cast<int32_t>(GPR[rs]) + static_cast<int16_t>(imm);
            std::cout << "[EXEC] ADDI $" << rt << ", $" << rs
                      << ", imm=" << std::dec << static_cast<int16_t>(imm) << "\n";
            break;

        case 0x0D: // ORI
            GPR[rt] = GPR[rs] | imm;
            std::cout << "[EXEC] ORI $" << rt << ", $" << rs
                      << ", 0x" << std::hex << imm << std::dec << "\n";
            break;

        case 0x23: { // LW
            uint32_t addr = static_cast<uint32_t>(GPR[rs] + static_cast<int16_t>(imm));
            uint32_t val = memory.read32(addr);
            GPR[rt] = val;
            std::cout << "[EXEC] LW $" << rt << ", 0x" << std::hex << imm
                      << "($" << rs << ") -> 0x" << val << std::dec << "\n";
            break;
        }

        case 0x2B: { // SW
            uint32_t addr = static_cast<uint32_t>(GPR[rs] + static_cast<int16_t>(imm));
            // For now, we only log store (memory write unimplemented)
            std::cout << "[EXEC] SW $" << rt << ", 0x" << std::hex << imm
                      << "($" << rs << ") -> STORE 0x" << GPR[rt] << std::dec << "\n";
            break;
        }

        default:
            std::cout << "[WARN] Unimplemented opcode: 0x"
                      << std::hex << opcode << std::dec << "\n";
            break;
    }
}

// -----------------------------------------------------------
// Safe Instruction Fetch Wrapper
// -----------------------------------------------------------
uint32_t CPU::fetch32(uint64_t addr) {
    try {
        return memory.read32(static_cast<uint32_t>(addr));
    } catch (...) {
        std::cerr << "❌ [EXCEPTION] Invalid fetch @0x" << std::hex << addr << std::dec << "\n";
        return 0;
    }
}

// -----------------------------------------------------------
// CPU Core - Memory Write, Exceptions, and Tracing (Part 5)
// -----------------------------------------------------------

#include <stdexcept>

bool traceEnabled = true; // global flag for instruction tracing

// Basic memory write (stub for now)
void CPU::write32(uint32_t addr, uint32_t value) {
    // Currently we don’t modify memory since this is a read-only PROM region
    if (traceEnabled) {
        std::cout << "[WRITE] 0x" << std::hex << addr << " <- 0x" << value << std::dec << "\n";
    }

    // In the future: handle RAM mapping & device-mapped regions
    // For now: log only
}

// -----------------------------------------------------------
// Exception & Interrupt Handling
// -----------------------------------------------------------
void CPU::raiseException(const std::string& reason) {
    std::cerr << "❌ [EXCEPTION] " << reason << " @PC=0x"
              << std::hex << PC << std::dec << "\n";

    // In the real R10000, this would trigger an exception vector
    // For now, we just stop execution by throwing
    throw std::runtime_error("CPU Exception: " + reason);
}

void CPU::checkInterrupts() {
    // Placeholder for interrupt handling logic
    // Would check pending interrupt flags, compare priorities, etc.
    if (false) {
        raiseException("External Interrupt");
    }
}

// -----------------------------------------------------------
// Logging and Debug Utilities
// -----------------------------------------------------------
void CPU::enableTrace(bool enable) {
    traceEnabled = enable;
    std::cout << (enable ? "[TRACE] Enabled\n" : "[TRACE] Disabled\n");
}

void CPU::printState() const {
    std::cout << "PC: 0x" << std::hex << PC << std::dec << "\n";
    for (int i = 0; i < 8; ++i) {
        std::cout << "R" << std::setw(2) << std::setfill('0') << i << ": 0x"
                  << std::hex << GPR[i] << std::dec << "  ";
        if ((i + 1) % 4 == 0) std::cout << "\n";
    }
    std::cout << "HI: 0x" << std::hex << HI << "  LO: 0x" << LO << std::dec << "\n";
}

// -----------------------------------------------------------
// CPU Core - Constructor cleanup, cycle counter, FPU/TLB stubs
// (Part 6)
// -----------------------------------------------------------

#include <atomic>

// Cycle counter for simple performance/limiting control
static std::atomic<uint64_t> global_cycle_counter{0};

CPU::CPU(Memory& mem) : memory(mem) {
    // Constructor body already minimal in Part1; ensure consistent initialization
    reset();
    // Optional: disable trace by default for faster stepping
    traceEnabled = false;
}

// Destructor (if any cleanup required later)
CPU::~CPU() {
    // Placeholder: release resources if we add dynamic allocations later
}

// Cycle management
void CPU::tick(uint64_t cycles /*=1*/) {
    global_cycle_counter += cycles;
}

uint64_t CPU::cycles() const {
    return global_cycle_counter.load();
}

void CPU::resetCycles() {
    global_cycle_counter.store(0);
}

// Simple FPU & TLB stubs (placeholders for later implementation)
class FPUStub {
public:
    FPUStub() { }
    // placeholder methods
    void reset() { /* reset FPU state */ }
    uint64_t getStateChecksum() const { return 0xdeadbeef; }
};

class TLBStub {
public:
    TLBStub() { }
    // Very simple identity TLB helper for early dev (no real translation)
    uint32_t translate(uint32_t vaddr) {
        // For early emulator, assume identity mapping for KSEG0/KSEG1 or ROM mappings are preconfigured
        // In the future, implement full TLB entries and exception handling
        return vaddr;
    }
};

// Attach/initialize optional subsystems
void CPU::attachFPU() {
    if (!fpu) fpu = std::make_unique<FPUStub>();
    fpu->reset();
    std::cout << "[CPU] FPU stub attached\n";
}

void CPU::attachTLB() {
    if (!tlb) tlb = std::make_unique<TLBStub>();
    std::cout << "[CPU] TLB stub attached\n";
}

// Integration points called by instruction handlers when appropriate
void CPU::onMemoryAccess(uint32_t addr, bool isWrite) {
    // Hook for performance counters, profiling, or TLB checks
    (void)addr; (void)isWrite;
    // Example: enforce a small cycle cost per memory access
    tick(1);
}

// Safe wrapper for memory writes (invokes write32 in Part 5)
void CPU::store32(uint32_t addr, uint32_t val) {
    onMemoryAccess(addr, true);
    write32(addr, val); // write32 defined earlier (stubbed)
}

// Safe wrapper for memory reads (uses fetch32 in Part 4)
uint32_t CPU::load32(uint32_t addr) {
    onMemoryAccess(addr, false);
    return fetch32(addr);
}

// Debug: report small summary of CPU + subsystems
void CPU::reportSummary() const {
    std::cout << "[CPU SUMMARY] PC=0x" << std::hex << PC
              << " cycles=" << std::dec << cycles() << "\n";
    std::cout << "  FPU attached: " << (fpu ? "yes" : "no") << "\n";
    std::cout << "  TLB attached: " << (tlb ? "yes" : "no") << "\n";
}

// Data members for stubs (declared here because we're adding implementation parts)
std::unique_ptr<FPUStub> CPU::fpu = nullptr;
std::unique_ptr<TLBStub> CPU::tlb = nullptr;

// -----------------------------------------------------------
// CPU Core - More Instructions, MMIO hooks, and Sample Harness
// (Part 7)
// -----------------------------------------------------------

#include <functional>

// Define a tiny UART interface (minimal) so CPU can call it.
// If you already have a UART implementation elsewhere, adapt the attach method to accept that type.
struct SimpleUART {
    // write a single byte to UART (host stdout)
    virtual void putByte(uint8_t b) {
        (void)b;
        // default: do nothing
    }
    virtual ~SimpleUART() = default;
};

// CPU MMIO base(s) and helper
static constexpr uint32_t UART_BASE_ADDR = 0x1FC10000; // suggested MMIO base for UART
static constexpr uint32_t UART_SIZE = 0x1000;

void CPU::attachUART(std::shared_ptr<SimpleUART> u) {
    uart = u;
    std::cout << "[CPU] UART attached to CPU\n";
}

// mmioWrite: call from instruction handlers when a store targets an MMIO address
void CPU::mmioWrite(uint32_t addr, uint32_t value) {
    // Example: detect UART data register at UART_BASE_ADDR + 0x00
    if (addr >= UART_BASE_ADDR && addr < UART_BASE_ADDR + UART_SIZE) {
        uint32_t offset = addr - UART_BASE_ADDR;
        if (offset == 0x00) { // data register
            if (uart) uart->putByte(static_cast<uint8_t>(value & 0xff));
            if (traceEnabled) {
                std::cout << "[MMIO] UART DATA WRITE: 0x" << std::hex << (value & 0xff) << std::dec << "\n";
            }
            return;
        } else {
            if (traceEnabled) {
                std::cout << "[MMIO] UART write at offset 0x" << std::hex << offset << " val=0x" << value << std::dec << "\n";
            }
            return;
        }
    }

    // Unhandled MMIO region: just trace
    if (traceEnabled) {
        std::cout << "[MMIO] Unhandled MMIO write @0x" << std::hex << addr
                  << " val=0x" << value << std::dec << "\n";
    }
}

// mmioRead: simple MMIO read hook (optional)
uint32_t CPU::mmioRead(uint32_t addr) {
    if (addr >= UART_BASE_ADDR && addr < UART_BASE_ADDR + UART_SIZE) {
        uint32_t offset = addr - UART_BASE_ADDR;
        if (offset == 0x04) { // example STATUS register
            // bit0 = tx ready, bit1 = rx ready (simple)
            return 0x01;
        }
        return 0;
    }
    if (traceEnabled) {
        std::cout << "[MMIO] Unhandled MMIO read @0x" << std::hex << addr << std::dec << "\n";
    }
    return 0xffffffffu;
}

// Extend instruction set: shifts, branches, syscall (simple)
void CPU::handleSPECIAL(uint32_t instr) {
    uint32_t funct = instr & 0x3F;
    uint32_t rs = (instr >> 21) & 0x1F;
    uint32_t rt = (instr >> 16) & 0x1F;
    uint32_t rd = (instr >> 11) & 0x1F;
    uint32_t shamt = (instr >> 6) & 0x1F;

    switch (funct) {
        case 0x00: // SLL
            GPR[rd] = (static_cast<uint32_t>(GPR[rt]) << shamt);
            if (traceEnabled) std::cout << "[EXEC] SLL $" << rd << ", $" << rt << ", " << shamt << "\n";
            break;
        case 0x02: // SRL
            GPR[rd] = (static_cast<uint32_t>(GPR[rt]) >> shamt);
            if (traceEnabled) std::cout << "[EXEC] SRL $" << rd << ", $" << rt << ", " << shamt << "\n";
            break;
        case 0x03: // SRA
            GPR[rd] = (static_cast<int32_t>(GPR[rt]) >> shamt);
            if (traceEnabled) std::cout << "[EXEC] SRA $" << rd << ", $" << rt << ", " << shamt << "\n";
            break;
        case 0x08: // JR
            std::cout << "[EXEC] JR $" << rs << " -> Jump to 0x" << std::hex << GPR[rs] << std::dec << "\n";
            PC = static_cast<uint64_t>(GPR[rs]);
            break;
        case 0x0c: // SYSCALL (treat as halt for now)
            std::cout << "[EXEC] SYSCALL encountered - halting CPU (debug)\n";
            running = false;
            break;
        default:
            // For other SPECIAL functions, fall back to Part2 behavior (ADD, SUB, etc.)
            // Try to reuse previously defined cases (ADD/ADDU/SUB/AND/OR) by calling earlier handler?
            // If merged sequentially, earlier Part2 implementation will already exist; otherwise trace.
            std::cout << "[WARN] Unhandled SPECIAL funct=0x" << std::hex << funct << std::dec << "\n";
            break;
    }
}

// Branches: BEQ and BNE
void CPU::handleBranch(uint32_t instr, bool isBEQ) {
    int32_t imm = static_cast<int16_t>(instr & 0xFFFF);
    uint32_t rs = (instr >> 21) & 0x1F;
    uint32_t rt = (instr >> 16) & 0x1F;
    bool taken = false;
    if (isBEQ) taken = (static_cast<uint32_t>(GPR[rs]) == static_cast<uint32_t>(GPR[rt]));
    else       taken = (static_cast<uint32_t>(GPR[rs]) != static_cast<uint32_t>(GPR[rt]));

    if (traceEnabled) {
        std::cout << "[EXEC] " << (isBEQ ? "BEQ" : "BNE")
                  << " $"<< rs << ", $" << rt << ", offset=" << imm
                  << " -> " << (taken ? "TAKEN" : "not taken") << "\n";
    }
    if (taken) {
        // branch delay slot semantics: next PC = PC + 4 + (imm << 2)
        uint32_t newpc = static_cast<uint32_t>(PC + 4 + (static_cast<int32_t>(imm) << 2));
        PC = newpc - 4; // adjust because step() will add +4 after decodeExecute
    }
}

// Modify decodeExecute here to call handleBranch for opcodes 0x04 (BEQ) and 0x05 (BNE).
// If your code structure calls decodeExecute earlier, ensure that opcode cases 0x04/0x05 route here.
// Example (if not already present in decodeExecute):
//   case 0x04: handleBranch(instr, true); break; // BEQ
//   case 0x05: handleBranch(instr, false); break; // BNE

// ---------------------------
// Sample harness (commented)
// ---------------------------
// Example usage to wire Memory, UART and CPU in your main() (replace MyMemory / MyUART with your implementations):
//
//    // create memory (1MiB PROM) and load file (use your Memory class from main.cpp)
//    Memory prom(1024*1024);
//    prom.loadFile("../roms/ip30prom.rev4.9.bin");
//
//    // create CPU and attach prom (depending how you integrated Memory into CPU constructor)
//    CPU cpu(prom);
//
//    // create a simple UART adaptor that prints to stdout
//    struct StdoutUART : public SimpleUART {
//        void putByte(uint8_t b) override {
//            std::cout << static_cast<char>(b) << std::flush;
//        }
//    };
//    auto uart = std::make_shared<StdoutUART>();
//    cpu.attachUART(uart);
//
//    // enable trace for debugging
//    cpu.enableTrace(true);
//
//    // Run a few steps (this will fetch instructions from PROM at PC=0xBFC00000)
//    cpu.run(200);
//
// Note: real PROM code expects many more instructions and device behavior; this harness is for early testing.
//
// -----------------------------------------------------------
// CPU Core - Exceptions, Pipeline Skeleton, and Error Handling
// (Part 8)
// -----------------------------------------------------------

enum class ExceptionCode {
    Int,        // interrupt
    AdEL,       // address error load
    AdES,       // address error store
    Syscall,    // syscall
    Break,      // break instruction
    RI,         // reserved instruction
    Ov,         // arithmetic overflow
    None
};

// Basic exception raising helper
void CPU::raiseException(ExceptionCode code) {
    std::string name;
    switch (code) {
        case ExceptionCode::AdEL: name = "AddressErrorLoad"; break;
        case ExceptionCode::AdES: name = "AddressErrorStore"; break;
        case ExceptionCode::Syscall: name = "Syscall"; break;
        case ExceptionCode::Break: name = "Break"; break;
        case ExceptionCode::RI: name = "ReservedInstruction"; break;
        case ExceptionCode::Ov: name = "Overflow"; break;
        default: name = "Unknown"; break;
    }

    std::cerr << "[EXC] Exception: " << name << " at PC=0x"
              << std::hex << PC << std::dec << "\n";

    // For now, halt CPU when exception occurs
    running = false;
}

// -----------------------------------------------------------
// Instruction Fetch Helper with Alignment Check
// -----------------------------------------------------------
uint32_t CPU::fetchInstruction(uint64_t addr) {
    if (addr & 0x3) {
        raiseException(ExceptionCode::AdEL);
        return 0;
    }
    return mem.read32(static_cast<uint32_t>(addr));
}

// -----------------------------------------------------------
// Simplified Pipeline Stage Framework
// -----------------------------------------------------------
struct PipelineStage {
    uint64_t pc = 0;
    uint32_t instr = 0;
    bool valid = false;
};

void CPU::runPipeline(int cycles) {
    PipelineStage fetchStage, decodeStage, execStage;
    running = true;

    for (int i = 0; i < cycles && running; ++i) {
        // Simple 3-stage pipeline mock
        execStage = decodeStage;
        decodeStage = fetchStage;

        // Fetch new instruction
        fetchStage.pc = PC;
        fetchStage.instr = fetchInstruction(PC);
        fetchStage.valid = true;

        if (execStage.valid) {
            decodeExecute(execStage.instr);
        }

        PC += 4;
    }

    std::cout << "[CPU] Pipeline run completed (" << cycles << " cycles)\n";
}

// -----------------------------------------------------------
// Memory Access Check (for alignment and MMIO)
// -----------------------------------------------------------
uint32_t CPU::safeRead32(uint32_t addr) {
    if (addr & 0x3) {
        raiseException(ExceptionCode::AdEL);
        return 0;
    }

    // Check MMIO first
    if (addr >= 0x1FC00000) {
        return mmioRead(addr);
    }
    return mem.read32(addr);
}

void CPU::safeWrite32(uint32_t addr, uint32_t value) {
    if (addr & 0x3) {
        raiseException(ExceptionCode::AdES);
        return;
    }

    if (addr >= 0x1FC00000) {
        mmioWrite(addr, value);
        return;
    }
    mem.write32(addr, value);
}

// -----------------------------------------------------------
// Enhanced Trace: Dump instruction fields
// -----------------------------------------------------------
void CPU::traceInstruction(uint32_t instr) {
    uint32_t opcode = (instr >> 26) & 0x3F;
    uint32_t rs = (instr >> 21) & 0x1F;
    uint32_t rt = (instr >> 16) & 0x1F;
    uint32_t rd = (instr >> 11) & 0x1F;
    uint32_t funct = instr & 0x3F;
    uint32_t imm = instr & 0xFFFF;

    std::cout << "[TRACE] PC=0x" << std::hex << PC << std::dec
              << " OPC=0x" << opcode
              << " rs=$" << rs << " rt=$" << rt << " rd=$" << rd
              << " imm=0x" << std::hex << imm << std::dec
              << " funct=0x" << std::hex << funct << std::dec << "\n";
}

// -----------------------------------------------------------
// Placeholder for future FPU integration
// -----------------------------------------------------------

void CPU::attachFPU(std::shared_ptr<void> fpuContext) {
    fpu = fpuContext;
    std::cout << "[CPU] Attached floating-point unit (stub)\n";
}

// -----------------------------------------------------------
// Future Steps (for Part 9):
//  - Add instruction cache / data cache emulation
//  - Implement TLB structure
//  - Begin support for IP30-specific PROM init patterns
// -----------------------------------------------------------

// -----------------------------------------------------------
// CPU Core - TLB, Translation, Caches, and IP30 Layout (Part 9)
// -----------------------------------------------------------

#include <vector>
#include <optional>
#include <mutex>

// ----------------------------
// Tiny TLB Implementation
// ----------------------------

struct TLBEntry {
    uint32_t vpage;     // virtual page number
    uint32_t ppage;     // physical page number
    uint32_t asid;      // address space id (not used initially)
    bool valid;
    bool writable;
    bool global;
    uint64_t last_used; // for simple LRU heuristics
};

class TinyTLB {
public:
    TinyTLB(size_t capacity = 64) : entries(capacity) {
        for (auto &e : entries) e.valid = false;
    }

    // Add/replace a TLB entry (simple linear search / replace-first-invalid)
    void addEntry(uint32_t vpage, uint32_t ppage, bool writable = false, uint32_t asid = 0) {
        std::lock_guard<std::mutex> lock(mtx);
        // try to find an existing mapping
        for (auto &e : entries) {
            if (e.valid && e.vpage == vpage && e.asid == asid) {
                e.ppage = ppage;
                e.writable = writable;
                e.last_used = usageCounter++;
                return;
            }
        }
        // find free slot
        for (auto &e : entries) {
            if (!e.valid) {
                e.vpage = vpage;
                e.ppage = ppage;
                e.asid = asid;
                e.valid = true;
                e.writable = writable;
                e.global = false;
                e.last_used = usageCounter++;
                return;
            }
        }
        // otherwise evict LRU
        size_t lru_idx = 0;
        uint64_t lru_val = UINT64_MAX;
        for (size_t i = 0; i < entries.size(); ++i) {
            if (entries[i].last_used < lru_val) {
                lru_val = entries[i].last_used;
                lru_idx = i;
            }
        }
        entries[lru_idx].vpage = vpage;
        entries[lru_idx].ppage = ppage;
        entries[lru_idx].asid = asid;
        entries[lru_idx].valid = true;
        entries[lru_idx].writable = writable;
        entries[lru_idx].last_used = usageCounter++;
    }

    // Translate virtual page -> physical page (returns nullopt if miss)
    std::optional<uint32_t> translate_page(uint32_t vpage, uint32_t asid = 0) {
        std::lock_guard<std::mutex> lock(mtx);
        for (auto &e : entries) {
            if (e.valid && e.vpage == vpage && (e.global || e.asid == asid)) {
                e.last_used = usageCounter++;
                return e.ppage;
            }
        }
        return std::nullopt;
    }

    void flushAll() {
        std::lock_guard<std::mutex> lock(mtx);
        for (auto &e : entries) e.valid = false;
    }

private:
    std::vector<TLBEntry> entries;
    std::mutex mtx;
    uint64_t usageCounter = 1;
};

// ----------------------------
// Tiny Cache Placeholders
// ----------------------------

class TinyCache {
public:
    TinyCache(size_t lines = 256, size_t lineBytes = 32) : line_size(lineBytes) {
        // no actual caching yet, placeholder structure
        (void)lines;
    }

    // Placeholder: mark address as cached (no-op)
    void markCached(uint32_t addr) { (void)addr; }

    // Placeholder: flush/clean (no-op)
    void flush() { /* nothing */ }
private:
    size_t line_size;
};

// ----------------------------
// IP30 / Octane PROM Layout Helper
// ----------------------------

// Common mapping assumptions for initial emulator:
//  - Map PROM file to virtual addresses starting at 0xBFC00000 (1 MiB region)
//  - For simple dev loop, assume KSEG0 identity mapping for low RAM
//  - Provide helper to set up CPU/memory mapping for PROM
static constexpr uint32_t IP30_PROM_VBASE = 0xBFC00000u;
static constexpr uint32_t IP30_PROM_SIZE  = 0x00100000u; // 1MiB

// Map an in-memory ROM buffer into the emulator's Memory object at the PROM base.
// This helper assumes the CPU has a reference to the memory object named `memory`
// which exposes a method to register a ROM region. If your memory API differs,
// adapt this function accordingly.
void CPU::mapIP30PROM(const std::vector<uint8_t>& rom_buffer) {
    // Attempt to register ROM into memory. If `memory` has an add_rom-like method, use it.
    // Otherwise, copy bytes into the Memory object at the PROM base.
    try {
        // If Memory has an add_rom method (in membus.h style), use that:
        // memory.add_rom(IP30_PROM_VBASE, rom_buffer);
        //
        // Otherwise, if Memory supports write into physical region, perform direct copy:
        for (size_t i = 0; i < rom_buffer.size() && i < IP30_PROM_SIZE; ++i) {
            // Assuming memory.write8 exists; if not, adapt to your API
            // Example safe write into ROM backing (this will need an API in Memory to accept direct writes)
            // memory.write8(IP30_PROM_VBASE + static_cast<uint32_t>(i), rom_buffer[i]);
            (void)rom_buffer; // prevent unused warning if above lines are commented
        }
        std::cout << "[CPU] IP30 PROM mapping helper invoked (size=" << rom_buffer.size() << ")\n";
    } catch (...) {
        std::cerr << "[CPU] Warning: mapIP30PROM failed - adapt to your Memory API\n";
    }
}

// ----------------------------
// Address translation helper
// ----------------------------

// Translate a virtual address into a physical address using TLB and KSEG heuristics.
// For early emulator operation, use the following conservative rules:
//  - If vaddr in KSEG0 (0x8000_0000 .. 0x9FFF_FFFF): map to paddr = vaddr & 0x1FFF_FFFF (cached)
//  - If vaddr in KSEG1 (0xA000_0000 .. 0xBFFF_FFFF): map to paddr = vaddr & 0x1FFF_FFFF (uncached)
//  - If vaddr in 0xBFC0_0000 .. 0xBFCF_FFFF (PROM region): treat as direct physical backed by ROM
//  - Otherwise: consult TLB; if miss, default to identity mapping for simplicity (vaddr -> paddr)
uint32_t CPU::translateVAddr(uint32_t vaddr) {
    // check KSEG0
    if ((vaddr & 0xE0000000u) == 0x80000000u) {
        return vaddr & 0x1FFFFFFFu;
    }
    // KSEG1
    if ((vaddr & 0xE0000000u) == 0xA0000000u) {
        return vaddr & 0x1FFFFFFFu;
    }
    // PROM region
    if (vaddr >= IP30_PROM_VBASE && vaddr < (IP30_PROM_VBASE + IP30_PROM_SIZE)) {
        // For simplicity, map PROM virtual address to a physical region in the emulator that holds the ROM.
        // We assume memory read routines know how to read from the PROM virtual addresses already.
        return vaddr; // identity for now; your memory layer may require a different mapping
    }
    // TLB consult
    uint32_t vpage = vaddr >> 12;
    auto maybe_ppage = tlb ? tlb->translate_page(vpage) : std::optional<uint32_t>{};
    if (maybe_ppage.has_value()) {
        uint32_t ppage = maybe_ppage.value();
        uint32_t offset = vaddr & 0xFFFu;
        return (ppage << 12) | offset;
    }

    // default identity mapping fallback
    return vaddr;
}

// ----------------------------
// Attach / init helpers for TLB & caches
// ----------------------------
void CPU::initMMUStubs() {
    if (!tlb) tlb = std::make_unique<TinyTLB>(64);
    if (!icache) icache = std::make_unique<TinyCache>(256, 32);
    if (!dcache) dcache = std::make_unique<TinyCache>(512, 32);
    std::cout << "[CPU] MMU stubs initialized (TLB/ICache/DCache placeholders)\n";
}

// ----------------------------
// Members for TLB & cache (definitions)
// ----------------------------
std::unique_ptr<TinyTLB> CPU::tlb = nullptr;
std::unique_ptr<TinyCache> CPU::icache = nullptr;
std::unique_ptr<TinyCache> CPU::dcache = nullptr;

// -----------------------------------------------------------
// End of Part 9
// -----------------------------------------------------------

// -----------------------------------------------------------
// CPU Core - Integration Harness & final glue for cpu.cpp
// (Part 10)
// -----------------------------------------------------------
//
// This part provides a minimal integration harness that shows how to:
//  - Load PROM bytes from disk into a vector
//  - Call CPU::mapIP30PROM with the ROM buffer
//  - Attach a simple stdout UART to the CPU
//  - Initialize MMU stubs and run a short instruction stream for testing
//
// NOTE: adapt the Memory / mem / memory API names to match your project's exact
// Memory class. This file assumes CPU has:
//   - constructor CPU(Memory& mem) or CPU(mem)
//   - mapIP30PROM(const std::vector<uint8_t>&)
//   - attachUART(std::shared_ptr<SimpleUART>)
//   - initMMUStubs()
//   - run(int cycles)
//   - enableTrace(bool)
//
// If your names differ, change the calls accordingly.

#include <fstream>
#include <memory>
#include <vector>

// Minimal Stdout UART implementation using SimpleUART interface
struct StdoutUART : public SimpleUART {
    void putByte(uint8_t b) override {
        std::cout << static_cast<char>(b) << std::flush;
    }
};

// Helper: load file into buffer
static bool loadFileToBuffer(const std::string& path, std::vector<uint8_t>& out) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) {
        std::cerr << "[IO] Failed to open file: " << path << "\n";
        return false;
    }
    std::streamsize sz = f.tellg();
    f.seekg(0, std::ios::beg);
    out.resize(static_cast<size_t>(sz));
    if (!f.read(reinterpret_cast<char*>(out.data()), sz)) {
        std::cerr << "[IO] Failed to read file: " << path << "\n";
        return false;
    }
    std::cout << "[IO] Loaded " << path << " (" << sz << " bytes)\n";
    return true;
}

// Integration harness entry point (example use in main.cpp)
// Call this from your main after creating appropriate Memory object.
int cpu_integration_test(const std::string& prom_path) {
    try {
        std::vector<uint8_t> rombuf;
        if (!loadFileToBuffer(prom_path, rombuf)) return 1;

        // NOTE: Replace the following Memory creation with your project's Memory object.
        // Here we assume a Memory class named Memory exists and CPU expects it in constructor.
        Memory memory(1024 * 1024); // 1 MiB backing for PROM area (adapt as needed)
        // If your Memory has a method to import a ROM buffer directly, use it:
        // memory.loadFromBuffer(IP30_PROM_VBASE, rombuf);

        // Create CPU instance bound to memory
        CPU cpu(memory);

        // Map PROM into CPU/memory (attempt)
        cpu.mapIP30PROM(rombuf);

        // Attach tiny MMU / TLB stubs to avoid surprises
        cpu.initMMUStubs();

        // Attach stdout UART
        auto uart = std::make_shared<StdoutUART>();
        cpu.attachUART(uart);

        // Enable trace for early stages (toggle off if too verbose)
        cpu.enableTrace(true);

        // Run a short number of cycles to exercise PROM startup
        cpu.run(500); // adjust as needed for debugging

        // Report CPU summary
        cpu.reportSummary();

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[ERROR] cpu_integration_test failure: " << ex.what() << "\n";
        return 2;
    }
}

// End of Part 10 - continue in Part 11
// -----------------------------------------------------------
// CPU Core - MMU and TLB Stub Implementation
// (Part 11)
// -----------------------------------------------------------
//
// This part creates a simplified MMU with Translation Lookaside Buffer (TLB)
// to emulate virtual-to-physical mapping for the MIPS R10000-like CPU
// used in SGI Octane (IP30). Real implementation will require handling
// wired, random, and indexed entries, as well as ASID and global flags.
// For now, we stub out essential mechanisms.

#include <array>
#include <optional>
#include <iomanip>

struct TLBEntry {
    uint64_t vaddr;
    uint64_t paddr;
    uint32_t mask;
    bool valid;
    bool dirty;
    uint8_t asid;
};

class MMU {
public:
    MMU() {
        tlb.fill({});
        for (auto &t : tlb) {
            t.valid = false;
            t.dirty = false;
            t.mask = 0xFFF;
            t.asid = 0;
        }
        std::cout << "[MMU] Initialized 48-entry TLB stub.\n";
    }

    std::optional<uint64_t> translate(uint64_t vaddr, uint8_t asid = 0) {
        for (auto &entry : tlb) {
            if (!entry.valid) continue;
            if ((vaddr & ~entry.mask) == (entry.vaddr & ~entry.mask)) {
                if (entry.asid == asid || entry.asid == 0) {
                    uint64_t paddr = entry.paddr | (vaddr & entry.mask);
                    if (trace_enabled) {
                        std::cout << "[MMU] VA 0x" << std::hex << vaddr
                                  << " -> PA 0x" << paddr << " (ASID=" << std::dec
                                  << (int)asid << ")\n";
                    }
                    return paddr;
                }
            }
        }
        return std::nullopt;
    }

    void writeTLB(int idx, uint64_t vaddr, uint64_t paddr, uint32_t mask, bool valid, bool dirty, uint8_t asid) {
        if (idx < 0 || idx >= (int)tlb.size()) return;
        auto &entry = tlb[idx];
        entry.vaddr = vaddr;
        entry.paddr = paddr;
        entry.mask = mask;
        entry.valid = valid;
        entry.dirty = dirty;
        entry.asid = asid;
        if (trace_enabled) {
            std::cout << "[MMU] TLB[" << idx << "] set: VA=0x" << std::hex << vaddr
                      << " -> PA=0x" << paddr << " mask=0x" << mask
                      << " valid=" << valid << " dirty=" << dirty
                      << " asid=" << std::dec << (int)asid << "\n";
        }
    }

    void dumpTLB() const {
        std::cout << "[MMU] Dumping TLB entries:\n";
        for (int i = 0; i < (int)tlb.size(); ++i) {
            const auto &t = tlb[i];
            if (!t.valid) continue;
            std::cout << "  [" << i << "] VA=0x" << std::hex << t.vaddr
                      << " -> PA=0x" << t.paddr
                      << " mask=0x" << t.mask
                      << " asid=" << std::dec << (int)t.asid
                      << " dirty=" << t.dirty << "\n";
        }
    }

    void setTrace(bool enable) { trace_enabled = enable; }

private:
    std::array<TLBEntry, 48> tlb;
    bool trace_enabled = false;
};

// Integration into CPU (stub linkage)
void CPU::initMMUStubs() {
    this->mmu = std::make_unique<MMU>();
    std::cout << "[CPU] MMU stub attached.\n";
}

std::optional<uint64_t> CPU::translateAddress(uint64_t vaddr, uint8_t asid) {
    if (!mmu) return vaddr; // Identity map if MMU off
    return mmu->translate(vaddr, asid);
}

void CPU::enableMMUTrace(bool enable) {
    if (mmu) mmu->setTrace(enable);
}

// End of Part 11 - continue in Part 12
// -----------------------------------------------------------
// CPU Core - Memory Access and Exception Hooks
// (Part 12)
// -----------------------------------------------------------

#include <stdexcept>

// Address translation helper
uint32_t CPU::read32(uint64_t vaddr) {
    auto phys = translateAddress(vaddr);
    if (!phys.has_value()) {
        raiseException(EXC_TLBL, vaddr);
        return 0;
    }
    return memory.read32(*phys);
}

void CPU::write32(uint64_t vaddr, uint32_t value) {
    auto phys = translateAddress(vaddr);
    if (!phys.has_value()) {
        raiseException(EXC_TLBS, vaddr);
        return;
    }
    memory.write32(*phys, value);
}

uint64_t CPU::read64(uint64_t vaddr) {
    auto phys = translateAddress(vaddr);
    if (!phys.has_value()) {
        raiseException(EXC_TLBL, vaddr);
        return 0;
    }
    uint64_t lo = memory.read32(*phys);
    uint64_t hi = memory.read32(*phys + 4);
    return (hi << 32) | lo;
}

void CPU::write64(uint64_t vaddr, uint64_t value) {
    auto phys = translateAddress(vaddr);
    if (!phys.has_value()) {
        raiseException(EXC_TLBS, vaddr);
        return;
    }
    memory.write32(*phys, (uint32_t)(value & 0xFFFFFFFF));
    memory.write32(*phys + 4, (uint32_t)(value >> 32));
}

// -----------------------------------------------------------
// Exception Handling (Placeholder)
// -----------------------------------------------------------
void CPU::raiseException(int code, uint64_t badvaddr) {
    std::cerr << "[EXCEPTION] Code=" << code
              << " BadVAddr=0x" << std::hex << badvaddr
              << " EPC=0x" << PC << std::dec << "\n";

    // In a real R10000, this would:
    //  - Save EPC
    //  - Modify SR (Status Register)
    //  - Jump to exception vector (0x80000000 + offset)
    // Here we simply stop execution for now.
    halted = true;
}

// -----------------------------------------------------------
// Opcode Dispatch Hook Integration
// -----------------------------------------------------------

void CPU::executeLoad(uint32_t opcode, uint64_t vaddr, int rt) {
    switch (opcode & 0x3F) {
        case 0x23: // LW
            setReg(rt, (int32_t)read32(vaddr));
            break;
        case 0x2B: // SW
            write32(vaddr, (uint32_t)getReg(rt));
            break;
        case 0x37: // LD (64-bit)
            setReg(rt, read64(vaddr));
            break;
        case 0x3F: // SD
            write64(vaddr, getReg(rt));
            break;
        default:
            std::cerr << "[WARN] Unsupported load/store opcode 0x"
                      << std::hex << (opcode & 0x3F) << std::dec << "\n";
            break;
    }
}

// -----------------------------------------------------------
// CPU Reset Hook Enhancement (MMU Integration)
// -----------------------------------------------------------
void CPU::fullReset() {
    reset();
    initMMUStubs();
    std::cout << "[CPU] Full system reset complete (with MMU).\n";
}

// (paste here code in Part 13)
// -----------------------------------------------------------
// CPU Core - CP0 (Coprocessor 0) Stubs and Exception Vectors
// (Part 13)
// -----------------------------------------------------------

#include <cstdint>
#include <iostream>
#include <iomanip>

// Simple CP0 register layout (very small subset)
struct CP0 {
    uint32_t Status = 0x00000000;   // Processor Status
    uint32_t Cause  = 0x00000000;   // Exception Cause
    uint32_t EPC    = 0x00000000;   // Exception Program Counter
    uint32_t BadVAddr = 0x00000000; // Bad Virtual Address
    uint32_t Config = 0x00000000;   // Config register stub
    // add other CP0 registers later as needed
};

// Exception vector addresses (MIPS-style simple mapping)
static constexpr uint32_t EXC_VEC_GENERAL = 0x80000080u; // general exception base (example)
static constexpr uint32_t EXC_VEC_RESET   = 0xBFC00000u; // reset vector (PROM area)
static constexpr uint32_t EXC_VEC_BOOT    = 0xBFC00000u; // boot/prom entry (alias for reset)

// Attach a CP0 instance to the CPU and provide read/write helpers
void CPU::initCP0() {
    if (!cp0) cp0 = std::make_unique<CP0>();
    std::cout << "[CPU] CP0 stub initialized\n";
}

uint32_t CPU::cp0_read(unsigned regnum) const {
    if (!cp0) return 0;
    switch (regnum) {
        case 12: return cp0->Status;    // SR
        case 13: return cp0->Cause;     // Cause
        case 14: return cp0->EPC;       // EPC
        case 8:  return cp0->BadVAddr;  // BadVAddr (not standard index but useful)
        case 16: return cp0->Config;    // Config
        default:
            std::cerr << "[CP0] Read from unimplemented CP0 reg " << regnum << "\n";
            return 0;
    }
}

void CPU::cp0_write(unsigned regnum, uint32_t value) {
    if (!cp0) initCP0();
    switch (regnum) {
        case 12:
            cp0->Status = value;
            break;
        case 13:
            cp0->Cause = value;
            break;
        case 14:
            cp0->EPC = value;
            break;
        case 8:
            cp0->BadVAddr = value;
            break;
        case 16:
            cp0->Config = value;
            break;
        default:
            std::cerr << "[CP0] Write to unimplemented CP0 reg " << regnum
                      << " value=0x" << std::hex << value << std::dec << "\n";
            break;
    }
}

// Exception helper: set CP0 fields and vector PC to exception handler
void CPU::handleException(ExceptionCode code, uint64_t badvaddr = 0) {
    if (!cp0) initCP0();

    // Save EPC (address of faulting instruction)
    cp0->EPC = static_cast<uint32_t>(PC);

    // Set BadVAddr if provided
    if (badvaddr) {
        cp0->BadVAddr = static_cast<uint32_t>(badvaddr);
    }

    // Set Cause based on ExceptionCode
    switch (code) {
        case ExceptionCode::AdEL:
            cp0->Cause = 4; // example: AdEL
            break;
        case ExceptionCode::AdES:
            cp0->Cause = 5;
            break;
        case ExceptionCode::Syscall:
            cp0->Cause = 8;
            break;
        case ExceptionCode::Break:
            cp0->Cause = 9;
            break;
        case ExceptionCode::RI:
            cp0->Cause = 10;
            break;
        case ExceptionCode::Ov:
            cp0->Cause = 12;
            break;
        default:
            cp0->Cause = 0;
            break;
    }

    std::cerr << "[EXC] Handling exception (cause=" << cp0->Cause
              << ") EPC=0x" << std::hex << cp0->EPC
              << " BadVAddr=0x" << cp0->BadVAddr << std::dec << "\n";

    // For this early emulator, jump to PROM/exception vector at EXC_VEC_GENERAL or PROM reset
    // Many MIPS systems map reset/boot vectors at 0xBFC00000; use that for PROM code.
    // If the Status register indicates bootstrap mode, use boot vector.
    uint32_t target_vec = EXC_VEC_GENERAL;

    // If exception occurred during reset/boot region, prefer boot vector
    if ((cp0->EPC >= EXC_VEC_BOOT && cp0->EPC < (EXC_VEC_BOOT + 0x00200000u))) {
        target_vec = EXC_VEC_BOOT;
    }

    // Set PC to exception vector (simulate branch to handler)
    PC = static_cast<uint64_t>(target_vec);
    running = true; // ensure CPU continues at vector (handler must clear)
}

// CP0 storage in CPU
std::unique_ptr<CP0> CPU::cp0 = nullptr;

// -----------------------------------------------------------
// CPU Core - State Serialization, Debug Utilities
// (Part 14)
// -----------------------------------------------------------

#include <fstream>
#include <string>

// Simple CPU state snapshot (very small subset)
struct CPUStateSnapshot {
    uint64_t pc;
    uint64_t gpr[32];
    uint64_t hi;
    uint64_t lo;
};

// Save minimal CPU state to a file (binary)
bool CPU::saveState(const std::string &path) const {
    CPUStateSnapshot snap;
    snap.pc = PC;
    for (int i = 0; i < 32; ++i) snap.gpr[i] = GPR[i];
    snap.hi = HI;
    snap.lo = LO;

    std::ofstream f(path, std::ios::binary);
    if (!f) {
        std::cerr << "[STATE] Failed to open snapshot file for writing: " << path << "\n";
        return false;
    }
    f.write(reinterpret_cast<const char*>(&snap), sizeof(snap));
    std::cout << "[STATE] Saved CPU snapshot to " << path << "\n";
    return true;
}

// Load minimal CPU state from file (binary)
bool CPU::loadState(const std::string &path) {
    CPUStateSnapshot snap;
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        std::cerr << "[STATE] Failed to open snapshot file for reading: " << path << "\n";
        return false;
    }
    f.read(reinterpret_cast<char*>(&snap), sizeof(snap));
    if (!f) {
        std::cerr << "[STATE] Failed to read full snapshot from " << path << "\n";
        return false;
    }
    PC = snap.pc;
    for (int i = 0; i < 32; ++i) GPR[i] = snap.gpr[i];
    HI = snap.hi;
    LO = snap.lo;
    std::cout << "[STATE] Restored CPU snapshot from " << path << "\n";
    return true;
}

// Scoped trace guard: enables trace for lifetime of object, then restores previous state
class ScopedTrace {
public:
    ScopedTrace(bool enable) {
        prev = traceEnabled;
        CPU::enableGlobalTrace(enable);
    }
    ~ScopedTrace() {
        CPU::enableGlobalTrace(prev);
    }
private:
    bool prev;
};

// Global enabling/disabling trace (affects all CPU instances if using static trace flag)
void CPU::enableGlobalTrace(bool enable) {
    traceEnabled = enable;
    std::cout << "[TRACE] Global trace " << (enable ? "ENABLED" : "DISABLED") << "\n";
}

// Sanity check helper (ensure CPU has required attachments)
void CPU::sanityCheck() const {
    if (!mmu) {
        std::cerr << "[SANITY] Warning: MMU not attached. Translations will be identity.\n";
    }
    if (!uart && traceEnabled) {
        std::cerr << "[SANITY] Note: UART not attached; PROM console output will be lost.\n";
    }
    if (!memory.isInitialized()) {
        std::cerr << "[SANITY] Warning: Memory subsystem not fully initialized.\n";
    }
}

// Small utility to print current program counter and nearby instructions (for debugging)
void CPU::dumpAroundPC(int before, int after) const {
    uint32_t start = static_cast<uint32_t>(PC) - (before * 4);
    for (int i = -before; i <= after; ++i) {
        uint32_t addr = static_cast<uint32_t>(PC) + i * 4;
        uint32_t instr = 0;
        try {
            instr = memory.read32(addr);
        } catch (...) {
            instr = 0xffffffff;
        }
        std::cout << (i == 0 ? "-> " : "   ")
                  << std::hex << addr << ": " << std::setw(8) << instr << std::dec << "\n";
    }
}

// End of Part 14 - continue in Part 15
// -----------------------------------------------------------
// CPU Core - Device Bus, Boot Entry, and Integration Hooks
// (Part 15)
// -----------------------------------------------------------

#include "bus.hpp"
#include "prom.hpp"
#include "uart.hpp"

// Attach device bus (for IO and memory access mapping)
void CPU::attachBus(Bus* busPtr) {
    bus = busPtr;
    if (bus)
        std::cout << "[CPU] Attached system bus.\n";
    else
        std::cerr << "[CPU] Detached system bus (NULL assigned).\n";
}

// Attach UART console device
void CPU::attachUART(UART* uartPtr) {
    uart = uartPtr;
    if (uart)
        std::cout << "[CPU] UART console attached.\n";
    else
        std::cerr << "[CPU] UART console detached (NULL assigned).\n";
}

// Attach PROM (boot ROM)
void CPU::attachPROM(PROM* promPtr) {
    prom = promPtr;
    if (prom)
        std::cout << "[CPU] PROM attached (revision " << prom->getRevision() << ").\n";
    else
        std::cerr << "[CPU] PROM detached.\n";
}

// Set reset vector (default 0x1FC00000 for SGI IP30)
void CPU::setResetVector(uint64_t vec) {
    resetVector = vec;
}

// Perform a full boot sequence up to PROM entry point
void CPU::boot() {
    sanityCheck();
    std::cout << "[BOOT] Booting from PROM at 0x" << std::hex << resetVector << std::dec << "...\n";

    if (!prom) {
        std::cerr << "[BOOT] No PROM attached. Cannot continue boot.\n";
        halted = true;
        return;
    }

    // Load first few instructions from PROM
    PC = resetVector;
    NPC = PC + 4;
    halted = false;

    // Simulate PROM console message if UART is connected
    if (uart) {
        uart->writeString("Speedracer PROM Boot: Starting execution...\n");
    }

    // Execute a small number of steps before halting (placeholder)
    for (int i = 0; i < 8 && !halted; ++i) {
        step();
    }

    std::cout << "[BOOT] PROM boot stub finished.\n";
}

// Run until halt or maxCycles reached
void CPU::runUntilHalt(uint64_t maxCycles) {
    std::cout << "[CPU] Running until halt (limit=" << maxCycles << ")...\n";
    uint64_t count = 0;
    while (!halted && count < maxCycles) {
        step();
        ++count;
    }
    std::cout << "[CPU] Execution stopped after " << count << " cycles.\n";
}

// Soft stop (requested externally)
void CPU::requestHalt(const std::string &reason) {
    std::cout << "[HALT] " << reason << "\n";
    halted = true;
}

// Reset CPU and jump to reset vector
void CPU::hardReset() {
    reset();
    PC = resetVector;
    NPC = PC + 4;
    halted = false;
    std::cout << "[RESET] Hard reset complete. PC=0x" << std::hex << PC << std::dec << "\n";
}

// Verify CPU connections before starting
bool CPU::verifyConnections() const {
    return (bus && memory.isInitialized() && prom);
}

// Log minimal system summary
void CPU::printSummary() const {
    std::cout << "--------------------------------------------\n";
    std::cout << " Speedracer Emulator - SGI Octane1 (IP30)\n";
    std::cout << " CPU Type: MIPS R10000 Compatible\n";
    std::cout << " PROM: " << (prom ? prom->getRevision() : "none") << "\n";
    std::cout << " Bus: " << (bus ? "connected" : "not connected") << "\n";
    std::cout << " Memory: " << (memory.isInitialized() ? "initialized" : "missing") << "\n";
    std::cout << "--------------------------------------------\n";
}

// End of Part 15 - continue in Part 16
// -----------------------------------------------------------
// CPU Core - Cycle Timing and Cache Hooks
// (Part 16)
// -----------------------------------------------------------

#include <chrono>

// -----------------------------------------------------------
// Cycle Counter & Timing Simulation
// -----------------------------------------------------------

void CPU::resetCycleCounter() {
    cycleCounter = 0;
    startTime = std::chrono::steady_clock::now();
}

void CPU::tick(uint64_t cycles) {
    cycleCounter += cycles;
    if (cycleCounter % 1000000 == 0) {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - startTime;
        double mhz = (cycleCounter / 1'000'000.0) / elapsed.count();
        std::cout << "[TIMER] " << cycleCounter << " cycles executed (" 
                  << mhz << " MHz simulated)\n";
    }
}

uint64_t CPU::getCycleCount() const {
    return cycleCounter;
}

// -----------------------------------------------------------
// Cache Subsystem (Simplified Placeholder)
// -----------------------------------------------------------

struct CacheLine {
    uint64_t tag = 0;
    bool valid = false;
    uint8_t data[64];
};

class SimpleCache {
public:
    SimpleCache(std::string name, size_t lines)
        : cacheName(std::move(name)), cacheLines(lines) {
        table.resize(lines);
    }

    bool fetch(uint64_t addr, uint8_t* buffer) {
        size_t index = (addr >> 6) % table.size();
        auto &line = table[index];
        if (line.valid && ((line.tag << 6) == (addr & ~0x3F))) {
            std::memcpy(buffer, line.data, 64);
            return true;
        }
        return false;
    }

    void store(uint64_t addr, const uint8_t* buffer) {
        size_t index = (addr >> 6) % table.size();
        auto &line = table[index];
        line.tag = (addr >> 6);
        line.valid = true;
        std::memcpy(line.data, buffer, 64);
    }

    void invalidateAll() {
        for (auto &l : table) l.valid = false;
    }

    void printStats() const {
        std::cout << "[" << cacheName << "] "
                  << table.size() << " lines total.\n";
    }

private:
    std::string cacheName;
    size_t cacheLines;
    std::vector<CacheLine> table;
};

// -----------------------------------------------------------
// Integration of Cache into CPU
// -----------------------------------------------------------

void CPU::initCaches() {
    icache = std::make_unique<SimpleCache>("ICache", 512);
    dcache = std::make_unique<SimpleCache>("DCache", 512);
    std::cout << "[CACHE] Instruction and Data caches initialized.\n";
}

void CPU::flushCaches() {
    if (icache) icache->invalidateAll();
    if (dcache) dcache->invalidateAll();
    std::cout << "[CACHE] All caches invalidated.\n";
}

void CPU::dumpCacheInfo() const {
    if (icache) icache->printStats();
    if (dcache) dcache->printStats();
}

// -----------------------------------------------------------
// Hooks for Cache Access in Load/Store
// -----------------------------------------------------------

bool CPU::tryICache(uint64_t addr, uint32_t &instr) {
    if (!icache) return false;
    uint8_t buf[64];
    if (!icache->fetch(addr, buf)) return false;
    std::memcpy(&instr, buf + (addr & 0x3C), sizeof(uint32_t));
    return true;
}

void CPU::updateICache(uint64_t addr, uint32_t instr) {
    if (!icache) return;
    uint8_t buf[64];
    std::memcpy(buf + (addr & 0x3C), &instr, sizeof(uint32_t));
    icache->store(addr, buf);
}

bool CPU::tryDCacheRead(uint64_t addr, uint64_t &value) {
    if (!dcache) return false;
    uint8_t buf[64];
    if (!dcache->fetch(addr, buf)) return false;
    std::memcpy(&value, buf + (addr & 0x38), sizeof(uint64_t));
    return true;
}

void CPU::updateDCacheWrite(uint64_t addr, uint64_t value) {
    if (!dcache) return;
    uint8_t buf[64];
    std::memcpy(buf + (addr & 0x38), &value, sizeof(uint64_t));
    dcache->store(addr, buf);
}

// End of Part 16 - continue in Part 17
// -----------------------------------------------------------
// CPU Core - TLB (Translation Lookaside Buffer)
// (Part 17)
// -----------------------------------------------------------

#include <array>
#include <optional>

struct TLBEntry {
    uint64_t vaddr;
    uint64_t paddr;
    uint64_t mask;
    bool valid;
    bool dirty;
    bool global;
    bool write;
    bool cacheable;
};

class TLB {
public:
    TLB(size_t entries = 48) : table(entries) {}

    std::optional<uint64_t> translate(uint64_t vaddr, bool writeAccess) {
        for (const auto& entry : table) {
            if (!entry.valid) continue;

            uint64_t maskedVAddr = vaddr & ~entry.mask;
            uint64_t maskedEntryVAddr = entry.vaddr & ~entry.mask;

            if (maskedVAddr == maskedEntryVAddr) {
                if (writeAccess && !entry.write) {
                    std::cerr << "[TLB] Write access violation at VA=0x"
                              << std::hex << vaddr << std::dec << "\n";
                    return std::nullopt;
                }

                return entry.paddr | (vaddr & entry.mask);
            }
        }
        return std::nullopt;
    }

    void addEntry(uint64_t vaddr, uint64_t paddr, uint64_t mask, bool write = true) {
        for (auto &e : table) {
            if (!e.valid) {
                e.vaddr = vaddr;
                e.paddr = paddr;
                e.mask = mask;
                e.valid = true;
                e.write = write;
                e.global = false;
                e.dirty = false;
                e.cacheable = true;
                return;
            }
        }
        std::cerr << "[TLB] Full, unable to insert entry VA=0x" << std::hex << vaddr << std::dec << "\n";
    }

    void invalidate(uint64_t vaddr) {
        for (auto &e : table) {
            if (e.valid && (vaddr & ~e.mask) == (e.vaddr & ~e.mask)) {
                e.valid = false;
                std::cout << "[TLB] Invalidated entry VA=0x" << std::hex << vaddr << std::dec << "\n";
            }
        }
    }

    void invalidateAll() {
        for (auto &e : table) e.valid = false;
        std::cout << "[TLB] All entries invalidated.\n";
    }

    void dump() const {
        std::cout << "TLB Entries:\n";
        for (size_t i = 0; i < table.size(); ++i) {
            const auto &e = table[i];
            if (!e.valid) continue;
            std::cout << " " << i << ": VA=0x" << std::hex << e.vaddr
                      << " -> PA=0x" << e.paddr
                      << " mask=0x" << e.mask
                      << (e.write ? " W" : " R")
                      << (e.cacheable ? " C" : " N")
                      << (e.global ? " G" : "")
                      << std::dec << "\n";
        }
    }

private:
    std::vector<TLBEntry> table;
};

// -----------------------------------------------------------
// Integration with CPU Core
// -----------------------------------------------------------

void CPU::initTLB(size_t entries) {
    tlb = std::make_unique<TLB>(entries);
    std::cout << "[TLB] Initialized with " << entries << " entries.\n";
}

void CPU::flushTLB() {
    if (tlb) {
        tlb->invalidateAll();
    }
}

std::optional<uint64_t> CPU::translateAddress(uint64_t vaddr) {
    if (!tlb) return vaddr;  // identity mapping if no TLB

    bool writeAccess = (currentOpcode & 0x3F) == 0x2B || (currentOpcode & 0x3F) == 0x3F; // SW, SD
    auto phys = tlb->translate(vaddr, writeAccess);
    if (!phys) {
        std::cerr << "[MMU] TLB miss for VA=0x" << std::hex << vaddr << std::dec << "\n";
    }
    return phys;
}

void CPU::debugTLB() const {
    if (tlb) tlb->dump();
    else std::cout << "[TLB] Not initialized.\n";
}

// End of Part 17 - continue in Part 18
// -----------------------------------------------------------
// CPU Core - Extended SPECIAL ops, LOAD/STORE hookup, and
// helper plumbing (Part 18)
// -----------------------------------------------------------
//
// Additions in this part:
//  - Implement MULT/MULTU/DIV/DIVU, MFHI/MFLO/MTHI/MTLO
//  - Implement SLT/SLTU
//  - Ensure currentOpcode is tracked for MMU translate use
//  - Wire load/store opcodes to translateAddress/read32/write32 helpers
//  - Add a few utility helpers for sign/zero extensions
//
// Note: this part intentionally uses both `memory` and `mem` naming
// in places; adapt to whichever your project's Memory instance is called.
// -----------------------------------------------------------

#include <limits>

// Ensure currentOpcode exists (used by translateAddress/MMU)
uint32_t CPU::currentOpcode = 0; // Definition for the static/instance field used earlier

// Sign-extend a 16-bit immediate into 32-bit signed value, return as int32_t
static inline int32_t signExtend16(uint32_t imm) {
    return static_cast<int16_t>(imm & 0xFFFF);
}

// Zero-extend 16-bit immediate
static inline uint32_t zeroExtend16(uint32_t imm) {
    return imm & 0xFFFFu;
}

// Helper: safe fetch with I-cache cooperation and alignment check
uint32_t CPU::fetchInstrAt(uint64_t vaddr) {
    // Try icache first (if available)
    uint32_t instr = 0;
    if (tryICache(vaddr, instr)) {
        return instr;
    }

    // Alignment check
    if (vaddr & 0x3) {
        handleException(ExceptionCode::AdEL);
        return 0;
    }

    // Address translation (TLB/MMU)
    auto phys = translateAddress(vaddr);
    if (!phys) {
        // TLB miss handling above already printed; signal exception
        handleException(ExceptionCode::RI);
        return 0;
    }

    // Read from memory
    uint32_t fetched = memory.read32(static_cast<uint32_t>(*phys));
    // Update I-cache
    updateICache(vaddr, fetched);
    return fetched;
}

// Extended SPECIAL decoder (merge/augment previous SPECIAL handlers)
void CPU::handleSPECIAL_extended(uint32_t instr) {
    uint32_t funct = instr & 0x3F;
    uint32_t rs = (instr >> 21) & 0x1F;
    uint32_t rt = (instr >> 16) & 0x1F;
    uint32_t rd = (instr >> 11) & 0x1F;
    uint32_t shamt = (instr >> 6) & 0x1F;

    switch (funct) {
        // Arithmetic already handled earlier: ADD/ADDU/SUB/AND/OR ...
        // Multiply / Divide group
        case 0x18: { // MULT
            int64_t a = static_cast<int32_t>(GPR[rs]);
            int64_t b = static_cast<int32_t>(GPR[rt]);
            int64_t prod = a * b;
            HI = static_cast<uint64_t>((static_cast<uint64_t>(prod) >> 32) & 0xFFFFFFFFu);
            LO = static_cast<uint64_t>(static_cast<uint32_t>(prod & 0xFFFFFFFFu));
            if (traceEnabled) std::cout << "[EXEC] MULT $" << rs << ", $" << rt << " -> HI/LO\n";
            break;
        }
        case 0x19: { // MULTU
            uint64_t a = static_cast<uint32_t>(GPR[rs]);
            uint64_t b = static_cast<uint32_t>(GPR[rt]);
            uint64_t prod = a * b;
            HI = (prod >> 32) & 0xFFFFFFFFu;
            LO = prod & 0xFFFFFFFFu;
            if (traceEnabled) std::cout << "[EXEC] MULTU $" << rs << ", $" << rt << " -> HI/LO\n";
            break;
        }
        case 0x1A: { // DIV
            int32_t a = static_cast<int32_t>(GPR[rs]);
            int32_t b = static_cast<int32_t>(GPR[rt]);
            if (b == 0) {
                // Division by zero: result unspecified on many MIPS; set HI/LO as implementation chooses
                raiseException(ExceptionCode::RI);
            } else {
                LO = static_cast<uint32_t>(a / b);
                HI = static_cast<uint32_t>(a % b);
                if (traceEnabled) std::cout << "[EXEC] DIV $" << rs << ", $" << rt << " -> LO(div) HI(rem)\n";
            }
            break;
        }
        case 0x1B: { // DIVU
            uint32_t a = static_cast<uint32_t>(GPR[rs]);
            uint32_t b = static_cast<uint32_t>(GPR[rt]);
            if (b == 0) {
                raiseException(ExceptionCode::RI);
            } else {
                LO = static_cast<uint32_t>(a / b);
                HI = static_cast<uint32_t>(a % b);
                if (traceEnabled) std::cout << "[EXEC] DIVU $" << rs << ", $" << rt << "\n";
            }
            break;
        }

        // Move to/from HI/LO
        case 0x10: // MFHI
            setReg(rd, HI);
            if (traceEnabled) std::cout << "[EXEC] MFHI $" << rd << "\n";
            break;
        case 0x12: // MFLO
            setReg(rd, LO);
            if (traceEnabled) std::cout << "[EXEC] MFLO $" << rd << "\n";
            break;
        case 0x11: // MTHI
            setHI(getReg(rs));
            if (traceEnabled) std::cout << "[EXEC] MTHI $" << rs << "\n";
            break;
        case 0x13: // MTLO
            setLO(getReg(rs));
            if (traceEnabled) std::cout << "[EXEC] MTLO $" << rs << "\n";
            break;

        // Set on less than (signed / unsigned)
        case 0x2A: // SLT
            setReg(rd, (static_cast<int32_t>(GPR[rs]) < static_cast<int32_t>(GPR[rt])) ? 1 : 0);
            if (traceEnabled) std::cout << "[EXEC] SLT $" << rd << ", $" << rs << ", $" << rt << "\n";
            break;
        case 0x2B: // SLTU
            setReg(rd, (static_cast<uint32_t>(GPR[rs]) < static_cast<uint32_t>(GPR[rt])) ? 1 : 0);
            if (traceEnabled) std::cout << "[EXEC] SLTU $" << rd << ", $" << rs << ", $" << rt << "\n";
            break;

        // Previously implemented JR, SLL, SRL, SRA, SYSCALL etc. Fallback to generic handler
        default:
            // Call older handler (if present) or warn
            if (traceEnabled) {
                std::cout << "[WARN] SPECIAL (extended) unhandled funct=0x" << std::hex << funct << std::dec << "\n";
            }
            break;
    }
}

// Hook load/store opcodes to MMU-aware helpers.
// This will offload actual read/write to read32/write32 which perform translation & exception handling.
void CPU::handleLoadStore(uint32_t instr) {
    uint32_t opcode = (instr >> 26) & 0x3F;
    uint32_t rs = (instr >> 21) & 0x1F;
    uint32_t rt = (instr >> 16) & 0x1F;
    int32_t imm = static_cast<int16_t>(instr & 0xFFFF);
    uint64_t vaddr = static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(GPR[rs]) + imm));

    switch (opcode) {
        case 0x23: { // LW
            uint32_t val = read32(vaddr);
            setReg(rt, static_cast<uint32_t>(val));
            if (traceEnabled) {
                std::cout << "[EXEC] LW $" << rt << ", " << imm << "($" << rs << ") -> 0x"
                          << std::hex << val << std::dec << "\n";
            }
            break;
        }
        case 0x2B: { // SW
            write32(vaddr, static_cast<uint32_t>(getReg(rt)));
            if (traceEnabled) {
                std::cout << "[EXEC] SW $" << rt << ", " << imm << "($" << rs << ") <- 0x"
                          << std::hex << getReg(rt) << std::dec << "\n";
            }
            break;
        }
        case 0x21: { // LH
            uint32_t raw = read32(vaddr & ~0x3u); // read aligned word and pick half
            // determine halfword offset
            uint32_t half = (vaddr & 0x2u) ? (raw >> 16) & 0xFFFFu : raw & 0xFFFFu;
            int16_t sval = static_cast<int16_t>(half);
            setReg(rt, static_cast<int32_t>(sval));
            if (traceEnabled) std::cout << "[EXEC] LH $" << rt << ", " << imm << "($" << rs << ")\n";
            break;
        }
        case 0x25: { // LHU
            uint32_t raw = read32(vaddr & ~0x3u);
            uint32_t half = (vaddr & 0x2u) ? (raw >> 16) & 0xFFFFu : raw & 0xFFFFu;
            setReg(rt, half);
            if (traceEnabled) std::cout << "[EXEC] LHU $" << rt << ", " << imm << "($" << rs << ")\n";
            break;
        }
        case 0x29: { // SH
            uint32_t aligned = vaddr & ~0x3u;
            uint32_t raw = read32(aligned);
            uint16_t half = static_cast<uint16_t>(getReg(rt) & 0xFFFFu);
            if (vaddr & 0x2u) {
                raw = (raw & 0x0000FFFFu) | (static_cast<uint32_t>(half) << 16);
            } else {
                raw = (raw & 0xFFFF0000u) | static_cast<uint32_t>(half);
            }
            write32(aligned, raw);
            if (traceEnabled) std::cout << "[EXEC] SH $" << rt << ", " << imm << "($" << rs << ")\n";
            break;
        }
        case 0x20: { // LB
            uint32_t raw = read32(vaddr & ~0x3u);
            uint32_t byteShift = (vaddr & 0x3u) * 8;
            int8_t b = static_cast<int8_t>((raw >> byteShift) & 0xFFu);
            setReg(rt, static_cast<int32_t>(b));
            if (traceEnabled) std::cout << "[EXEC] LB $" << rt << ", " << imm << "($" << rs << ")\n";
            break;
        }
        case 0x24: { // LBU
            uint32_t raw = read32(vaddr & ~0x3u);
            uint32_t byteShift = (vaddr & 0x3u) * 8;
            uint8_t b = static_cast<uint8_t>((raw >> byteShift) & 0xFFu);
            setReg(rt, b);
            if (traceEnabled) std::cout << "[EXEC] LBU $" << rt << ", " << imm << "($" << rs << ")\n";
            break;
        }
        case 0x28: { // SB
            uint32_t aligned = vaddr & ~0x3u;
            uint32_t raw = read32(aligned);
            uint32_t byteShift = (vaddr & 0x3u) * 8;
            uint32_t mask = ~(0xFFu << byteShift);
            uint32_t newraw = (raw & mask) | ((static_cast<uint32_t>(getReg(rt) & 0xFFu) << byteShift));
            write32(aligned, newraw);
            if (traceEnabled) std::cout << "[EXEC] SB $" << rt << ", " << imm << "($" << rs << ")\n";
            break;
        }
        default:
            std::cout << "[WARN] Unhandled load/store opcode 0x" << std::hex << opcode << std::dec << "\n";
            break;
    }
}

// Top-level opcode dispatcher augmentation to integrate SPECIAL extended and loads/stores
void CPU::decodeExecute_augmented(uint32_t instr) {
    // track current opcode for MMU heuristics
    currentOpcode = instr;

    uint32_t opcode = (instr >> 26) & 0x3F;

    switch (opcode) {
        case 0x00: // SPECIAL - route to extended handler
            handleSPECIAL_extended(instr);
            break;

        // Branches
        case 0x04: // BEQ
            handleBranch(instr, true);
            break;
        case 0x05: // BNE
            handleBranch(instr, false);
            break;

        // Immediate arithmetic & logical ops (some already in earlier parts)
        case 0x08: // ADDI
        case 0x09: // ADDIU
        case 0x0C: // ANDI
        case 0x0D: // ORI
        case 0x0E: // XORI
        case 0x0A: // SLTI
        case 0x0B: // SLTIU
            // Many immediates were implemented earlier; fall through to existing decodeExecute
            // If older decodeExecute already handles them, leave as-is. Otherwise, implement minimal support:
            {
                uint32_t rs = (instr >> 21) & 0x1F;
                uint32_t rt = (instr >> 16) & 0x1F;
                uint32_t imm = instr & 0xFFFFu;
                switch (opcode) {
                    case 0x08: // ADDI
                        setReg(rt, static_cast<int32_t>(getReg(rs)) + static_cast<int32_t>(signExtend16(imm)));
                        break;
                    case 0x09: // ADDIU
                        setReg(rt, static_cast<uint32_t>(getReg(rs)) + zeroExtend16(imm));
                        break;
                    case 0x0C: // ANDI
                        setReg(rt, getReg(rs) & zeroExtend16(imm));
                        break;
                    case 0x0D: // ORI
                        setReg(rt, getReg(rs) | zeroExtend16(imm));
                        break;
                    case 0x0E: // XORI
                        setReg(rt, getReg(rs) ^ zeroExtend16(imm));
                        break;
                    case 0x0A: // SLTI
                        setReg(rt, (static_cast<int32_t>(getReg(rs)) < static_cast<int32_t>(signExtend16(imm))) ? 1 : 0);
                        break;
                    case 0x0B: // SLTIU
                        setReg(rt, (static_cast<uint32_t>(getReg(rs)) < zeroExtend16(imm)) ? 1 : 0);
                        break;
                }
                if (traceEnabled) {
                    std::cout << "[EXEC] IMM opcode 0x" << std::hex << opcode << std::dec << "\n";
                }
            }
            break;

        // Loads & Stores
        case 0x23: // LW
        case 0x2B: // SW
        case 0x21: // LH
        case 0x25: // LHU
        case 0x29: // SH
        case 0x20: // LB
        case 0x24: // LBU
        case 0x28: // SB
            handleLoadStore(instr);
            break;

        // Jumps
        case 0x02:
            handleJ(instr);
            break;
        case 0x03:
            handleJAL(instr);
            break;

        default:
            std::cout << "[WARN] decodeExecute_augmented: Unhandled opcode 0x" << std::hex << opcode << std::dec << "\n";
            break;
    }
}

// Replace previous decodeExecute calls in run/step/pipeline to use the augmented dispatcher
// If your code earlier called decodeExecute(instr), search/replace to decodeExecute_augmented(instr)
// in the surrounding source files or call it explicitly at callsites.
//
// Example replacement (if using step()):
//    uint32_t instr = fetchInstrAt(PC);
//    decodeExecute_augmented(instr);
//    PC += 4;
//
// For pipeline: ensure exec stage uses decodeExecute_augmented(execStage.instr);

// End of Part 18
// -----------------------------------------------------------
// CPU Core - COP0, Exceptions, Branch/Delay handling, and
// some control-flow helpers (Part 19)
// -----------------------------------------------------------
//
// Additions in this part:
//  - Basic COP0 MFC0 / MTC0 handling (move from/to CP0 registers)
//  - ERET handling (return from exception) and exception entry helper
//  - Syscall & BREAK handling (raise appropriate exceptions)
//  - JALR & JR link/return handling and branch-delay bookkeeping
//  - Simple interrupt check hook (timer/async interrupts can call raiseHWInterrupt)
//  - Integration notes: wire these handlers into main dispatcher
//
// This part intentionally keeps implementations conservative so they
// can be adjusted to your CPU/CP0 layout and existing exception model.
//
// Assumptions (adapt as needed):
//  - CPU class has:
//      uint32_t GPR[32]; uint64_t PC, nextPC;
//      uint32_t CP0[32];         // or a CP0 map: adapt if different
//      bool inDelaySlot;        // branch-delay tracking
//      bool traceEnabled;
//      void handleException(ExceptionCode code, uint32_t info=0);
//      void raiseException(ExceptionCode code);
//      void raiseHWInterrupt(int irq); // external interrupt from devices
//      uint32_t getReg(unsigned idx), setReg(unsigned idx, uint32_t val)
//      uint32_t read32(uint64_t vaddr), void write32(uint64_t vaddr, uint32_t val)
//      void setPC(uint64_t addr), uint64_t getPC()
//  - ExceptionCode enum contains: Syscall, Break, AdEL, RI, Int, etc.
//  - ERET uses CP0 status/cause/epc semantics similar to MIPS
//
// If your project uses different names/structures, grep/replace accordingly.
//
// -----------------------------------------------------------

#include <cassert>

// Basic CP0 register access (MFC0 / MTC0)
// instr is the full 32-bit instruction (we read rd/rt fields same as SPECIAL layout)
void CPU::handleCOP0(uint32_t instr) {
    uint32_t rt = (instr >> 16) & 0x1F;
    uint32_t rd = (instr >> 11) & 0x1F; // cp0 register number or subfield
    uint32_t rs = (instr >> 21) & 0x1F; // usually selects operation (MFC0=0x00, MTC0=0x04)
    uint32_t opcode = (instr >> 26) & 0x3F;

    // COP0 primary opcode is usually 0x10 (coprocessor 0)
    // decode rs for MFC0/MTC0/BC0/COFUN
    switch (rs) {
        case 0x00: { // MFC0 - Move from CP0 to GPR
            uint32_t val = readCP0(rd);
            setReg(rt, val);
            if (traceEnabled) std::cout << "[EXEC] MFC0 $" << rt << ", CP0[" << rd << "] = 0x" << std::hex << val << std::dec << "\n";
            break;
        }
        case 0x04: { // MTC0 - Move to CP0 from GPR
            uint32_t val = getReg(rt);
            writeCP0(rd, val);
            if (traceEnabled) std::cout << "[EXEC] MTC0 CP0[" << rd << "] <- 0x" << std::hex << val << std::dec << "\n";
            break;
        }
        case 0x10: { // BC0x - Branch on COP0 condition (e.g., BC0F/BC0T)
            // treat as branch depending on a flag in CP0 (implementation-specific)
            // use rt low bit to discriminate (rt==0: BC0F, rt==1: BC0T)
            bool cond = (CP0ConditionFlag()); // implement according to your CP0 Cause/Status bits
            bool isTaken = ((instr >> 16) & 0x1) ? !cond : cond; // slightly defensive
            if (isTaken) {
                int16_t imm = static_cast<int16_t>(instr & 0xFFFF);
                uint64_t target = nextPC + (static_cast<int32_t>(imm) << 2);
                branchTo(target, true); // record delay slot
            } else {
                // not taken: still advance normally
            }
            if (traceEnabled) std::cout << "[EXEC] BC0 cond -> " << isTaken << "\n";
            break;
        }
        default:
            // For other coprocessor functions, dispatch by function field (bits 0..5)
            {
                uint32_t funct = instr & 0x3F;
                if (funct == 0x18) { // ERET (often encoded in COP0 function space)
                    doERET();
                    if (traceEnabled) std::cout << "[EXEC] ERET\n";
                } else {
                    if (traceEnabled) std::cout << "[WARN] COP0 unhandled rs=0x" << std::hex << rs << " funct=0x" << funct << std::dec << "\n";
                }
            }
            break;
    }
}

// Read / write CP0 helpers (abstracted to allow special handling of Status/Cause/EPC)
uint32_t CPU::readCP0(unsigned reg) {
    // Customize: some CP0 registers are 64-bit or partially reserved.
    // Minimal implementation: map into array CP0[32]
    assert(reg < 32);
    uint32_t val = CP0[reg];
    // Optionally mask read-only bits for Status/Cause
    return val;
}

void CPU::writeCP0(unsigned reg, uint32_t value) {
    assert(reg < 32);
    // Handle side-effects for certain CP0 regs (Status, Cause, EPC, Count/Compare)
    switch (reg) {
        case 9: // Count (example)
            CP0[9] = value;
            break;
        case 11: // Compare - writing clears timer interrupt in many MIPS cores
            CP0[11] = value;
            // clear timer interrupt bit in Cause/Status (implementation-defined)
            clearTimerInterrupt();
            break;
        case 12: // Status
            // mask reserved bits properly; don't allow userspace to toggle kernel-only flags incorrectly
            CP0[12] = value;
            break;
        case 13: // Cause (mostly read-only; software may write some bits)
            CP0[13] = value & 0x000000FFu; // allow only low bits to be modified in this minimal model
            break;
        case 14: // EPC
            CP0[14] = value;
            break;
        default:
            CP0[reg] = value;
            break;
    }
}

// ERET implementation: restore previous mode and PC from EPC.
// This is a minimal form; real machines must restore Status bits, interrupt masks.
void CPU::doERET() {
    // In MIPS: EPC is in CP0 register 14, Status contains mode bits
    uint32_t epc = CP0[14];
    // Clear EXL bit in Status (bit 1) to leave exception mode
    CP0[12] &= ~(1u << 1);
    // Set PC to EPC (note: EPC may point to the branch delay slot address; real cores sometimes adjust)
    setPC(static_cast<uint64_t>(epc));
    // After ERET, pipeline restarts; ensure nextPC is PC+4
    nextPC = PC + 4;
    inDelaySlot = false;
}

// Exception entry helper: save EPC/Cause/Status and enter exception vector.
void CPU::enterException(ExceptionCode code, uint32_t info) {
    // Save EPC: if exception happened in a branch delay slot, save PC-4 in EPC and set BD bit in Cause
    uint32_t epcToSave = static_cast<uint32_t>(PC);
    if (inDelaySlot) {
        // If exception occurred in delay slot, EPC should hold the branch's PC (PC - 4)
        CP0[13] |= (1u << 31); // set BD (Branch Delay) bit in Cause if you model it (bit positions vary)
        epcToSave = static_cast<uint32_t>(PC - 4);
    } else {
        CP0[13] &= ~(1u << 31);
    }
    CP0[14] = epcToSave; // EPC
    // Set Cause register with the exception code in bits 2..6 typically
    CP0[13] = (CP0[13] & ~0x7C) | ((static_cast<uint32_t>(code) & 0x1F) << 2);
    // Save Status: set EXL bit to enter exception mode
    CP0[12] |= (1u << 1); // EXL = 1
    // Jump to exception vector. For many MIPS: 0x80000080 or 0xBFC00180 for bootstrap; choose one per target.
    uint64_t vector = 0x80000080ULL; // SGI IP30 typical kernel vector; adapt if needed
    setPC(vector);
    nextPC = PC + 4;
    inDelaySlot = false;
    if (traceEnabled) std::cout << "[EXC] enterException code=" << static_cast<int>(code) << " EPC=0x" << std::hex << epcToSave << std::dec << "\n";
}

// Syscall & BREAK handling produce exceptions
void CPU::handleSyscallBreak(uint32_t instr) {
    uint32_t funct = instr & 0x3F;
    if ((instr >> 26) == 0x00 && funct == 0x0C) { // SYSCALL
        enterException(ExceptionCode::Syscall, 0);
        if (traceEnabled) std::cout << "[EXEC] SYSCALL -> exception\n";
    } else if ((instr >> 26) == 0x00 && funct == 0x0D) { // BREAK
        enterException(ExceptionCode::Break, 0);
        if (traceEnabled) std::cout << "[EXEC] BREAK -> exception\n";
    } else {
        // should not reach here
    }
}

// Branch / delay slot helper: perform the branch transfer, recording that the next
// instruction executed is in a delay slot (typical MIPS behaviour). `target` is branch target.
// If `link` is true, write PC+8 (per MIPS semantics) to link register (e.g., for JAL)
void CPU::branchTo(uint64_t target, bool isDelaySlotBranch, bool link, unsigned linkReg) {
    // In MIPS: the branch executes next instruction in the delay slot (PC+4),
    // but the branch target is loaded into PC after the delay slot instruction completes.
    // We implement by setting nextPC to target and marking that we are in a branch with delay slot.
    // Save link if requested (JAL/JALR)
    if (link) {
        uint32_t returnAddr = static_cast<uint32_t>(PC + 8); // PC is current instruction
        setReg(linkReg, returnAddr);
    }

    // Record branch target into a deferred slot and mark that we are in delay behavior.
    pendingBranchTarget = target;
    pendingBranchTaken = true;
    // The instruction immediately following (nextPC) will be executed with inDelaySlot==true
    // Ensure caller sets inDelaySlot before executing the following instruction.
}

// Call this after executing an instruction to apply any pending branch target
void CPU::applyPendingBranch() {
    if (pendingBranchTaken) {
        setPC(pendingBranchTarget);
        nextPC = PC + 4;
        pendingBranchTaken = false;
        inDelaySlot = false;
    } else {
        // normal fall-through
        setPC(nextPC);
        nextPC = PC + 4;
        inDelaySlot = false;
    }
}

// JALR & JR handling (JALR writes link to rd, default rd=31 for JALR with no explicit rd)
void CPU::handleJALR_JR(uint32_t instr) {
    uint32_t funct = instr & 0x3F;
    uint32_t rs = (instr >> 21) & 0x1F;
    uint32_t rd = (instr >> 11) & 0x1F;

    uint64_t target = static_cast<uint64_t>(getReg(rs)) & ~0x3u; // low 2 bits cleared
    if (funct == 0x09) { // JALR
        unsigned linkReg = (rd == 0) ? 31 : rd;
        branchTo(target, true, true, linkReg);
        if (traceEnabled) std::cout << "[EXEC] JALR $" << rd << " <- link, target=0x" << std::hex << target << std::dec << "\n";
    } else if (funct == 0x08) { // JR
        branchTo(target, true, false, 0);
        if (traceEnabled) std::cout << "[EXEC] JR target=0x" << std::hex << target << std::dec << "\n";
    } else {
        // not handled here
    }
}

// Very small interrupt test hook. Real implementations compare CP0 Status/Cause & HW lines.
void CPU::checkPendingInterrupts() {
    // If CP0 Status enables interrupts and CP0 Cause shows pending, raise an Int exception.
    // Minimal model: if any HW IRQ flag is set (we track hwInterruptMask/hwInterruptPending),
    // and EXL bit is clear, then take an interrupt.
    bool exl = (CP0[12] & (1u << 1)) != 0;
    if (exl) return; // already in exception handling

    if (hwInterruptPending & hwInterruptMask) {
        // set Cause IP bits accordingly (implementation-dependent)
        CP0[13] |= (hwInterruptPending & 0xFF) << 8; // example placement in Cause IP[15:8]
        enterException(ExceptionCode::Int, 0);
        if (traceEnabled) std::cout << "[INT] hardware interrupt -> exception\n";
    }
}

// Integration notes:
// - Call checkPendingInterrupts() periodically inside the main step/run loop
//   (for example once per instruction or once per N cycles).
// - Ensure decodeExecute_augmented includes calls for COP0 primary opcode (0x10).
//     case 0x10: handleCOP0(instr); break;
// - Ensure SPECIAL handler routes JALR/JR to handleJALR_JR when funct == 0x08/0x09,
//   or augment handleSPECIAL_extended accordingly.
//
// Example: in decodeExecute_augmented add:
//   case 0x10: handleCOP0(instr); break;
//
// Also ensure SYS/CALL and BREAK are dispatched somewhere (they are SPECIAL functs 0x0C/0x0D):
//   if (opcode==0x00 && (instr & 0x3F) in {0x0C,0x0D}) handleSyscallBreak(instr);
//
// End of Part 19
// (paste here code in Part 20)

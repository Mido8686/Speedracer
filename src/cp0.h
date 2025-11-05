#pragma once
#include <cstdint>
#include <array>

/*
  Minimal CP0 implementation suitable for an SGI IP30/MIPS emulator.
  Provides read/write helpers and a few convenience methods for status/cause/epc handling.

  This is intentionally small — expand fields & bit masks as needed.
*/

class CP0 {
public:
    CP0();

    // Basic read/write of CP0 registers (index 0..31)
    uint32_t read(unsigned reg) const;
    void write(unsigned reg, uint32_t value);

    // Convenience helpers
    void setEXL(bool v);          // EXL bit in Status
    bool getEXL() const;
    void setEPC(uint32_t v);
    uint32_t getEPC() const;
    void setCause(uint32_t v);
    uint32_t getCause() const;

    // Count/Compare behavior: call this per-instruction tick to increment Count,
    // compare to Compare and set timer interrupt bit if necessary.
    // Should be called by CPU/MMU per some cycles or by a timer helper.
    void tickCount();

    // mark/clear hw interrupt pending mask bits (IPx in Cause)
    void setHWPending(uint32_t mask);
    void clearHWPending(uint32_t mask);
    uint32_t getHWPending() const;

    // small public register array (use read/write where possible)
    std::array<uint32_t, 32> regs;

private:
    // Masks for commonly used CP0 regs (register numbers)
    static constexpr unsigned REG_INDEX   = 0;
    static constexpr unsigned REG_RANDOM  = 1;
    static constexpr unsigned REG_ENTRYLO0 = 2;
    static constexpr unsigned REG_ENTRYLO1 = 3;
    static constexpr unsigned REG_CONTEXT = 4;
    static constexpr unsigned REG_PAGEMASK = 5;
    static constexpr unsigned REG_WIRED   = 6;
    static constexpr unsigned REG_BADVADDR = 8;
    static constexpr unsigned REG_COUNT   = 9;
    static constexpr unsigned REG_COMPARE = 11;
    static constexpr unsigned REG_STATUS  = 12;
    static constexpr unsigned REG_CAUSE   = 13;
    static constexpr unsigned REG_EPC     = 14;
};

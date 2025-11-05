*** Begin Patch: src/cpu.h
*** Update File
@@
 #include <cstdint>
+#include <optional>
+#include "cp0.h"
+#include "mmu.h"
@@
 class CPU {
 public:
     CPU();
     void reset();
     void step(Memory &mem, Bus &bus); // single instruction/quantum
+    // --- New members for CP0 / MMU integration ---
+    CP0 cp0;               // coprocessor 0 state
+    MMU *mmu = nullptr;    // pointer to the MMU instance (owned by emulator)
+    Memory *memory = nullptr; // pointer to memory used by the CPU/MMU
+
+    // CP0 and translation helpers used by cpu.cpp
+    inline uint32_t readCP0(unsigned r) { return cp0.read(r); }
+    inline void writeCP0(unsigned r, uint32_t v) { cp0.write(r, v); }
+    // Translate virtual address through MMU (returns std::optional phys addr)
+    inline std::optional<uint64_t> translateAddress(uint64_t vaddr, unsigned accessSize = 4, bool isWrite = false) {
+        if (!mmu) return std::optional<uint64_t>(vaddr); // identity map if MMU missing
+        return mmu->translate(vaddr, accessSize, isWrite);
+    }
+
+    // TLB management primitives — these are implemented in cpu.cpp and call into mmu
+    // TLBR: read a TLB entry indexed by CP0 Index and populate CP0 EntryLo/PageMask/etc.
+    void tlbr();
+    // TLBWI: write TLB entry at CP0 Index from CP0 EntryLo/PageMask/etc.
+    void tlbwi();
+    // TLBWR: write TLB entry using MMU::writeRandom (writes at pseudo-random index)
+    void tlbwr();
+    // TLBP: probe the TLB for CP0 EntryLo/PageMask/etc value, set CP0 Index with result or -1
+    void tlbp();
@@
 private:
     uint64_t cycles = 0;
 };
*** End Patch

*** Begin Patch: src/mmu.h
*** Update File
@@
 #include <array>
 
 struct TLBEntry {
     bool valid = false;
     uint32_t vpn = 0;      // virtual page number (high bits of vaddr)
     uint32_t pfn = 0;      // physical frame number (high bits of paddr)
     uint32_t asid = 0;     // address space id
     bool global = false;
     bool dirty = true;
     bool validP = true;    // PFN valid
     uint32_t pageMask = 0x00000000; // small pages only for now
 };
 
 class Memory; // forward
 
 class MMU {
 public:
     MMU(Memory *mem, class CP0 *cp0, unsigned tlbSize = 64);
 
@@
     // Insert / update TLB entry (software-managed)
     void insertTLBEntry(unsigned index, const TLBEntry &e);
 
+    // TLB management helpers:
+    // Read entry at index (returns optional to indicate validity)
+    std::optional<TLBEntry> readTLBEntry(unsigned index) const;
+    // Write entry at index
+    void writeTLBEntry(unsigned index, const TLBEntry &e);
+    // Write entry using Random pointer (WRAP style). Returns the index written.
+    unsigned writeTLBRandom(const TLBEntry &e);
+    // Probe for entry matching vpn; returns index or -1 if not found
+    int probeTLB(uint32_t vpn) const;
+
     // Simple utilities
     void flushAll();
 
 private:
     Memory *memory;
     CP0 *cp0;
     std::vector<TLBEntry> tlb;
     unsigned tlbSize;
+    // Simple random pointer for TLBWR behaviour
+    unsigned randomPtr = 0;
 };
*** End Patch

*** Begin Patch: src/mmu.cpp
*** Update File
@@
 std::optional<uint64_t> MMU::translate(uint64_t vaddr, unsigned accessSize, bool isWrite) {
@@
     return std::nullopt;
 }
 
 void MMU::insertTLBEntry(unsigned index, const TLBEntry &e) {
@@
     tlb[index] = e;
 }
 
+std::optional<TLBEntry> MMU::readTLBEntry(unsigned index) const {
+    if (index >= tlb.size()) return std::nullopt;
+    return tlb[index];
+}
+
+void MMU::writeTLBEntry(unsigned index, const TLBEntry &e) {
+    if (index >= tlb.size()) {
+        std::cerr << "[MMU] writeTLBEntry: index out of range, wrapping\n";
+        index = index % tlb.size();
+    }
+    tlb[index] = e;
+}
+
+unsigned MMU::writeTLBRandom(const TLBEntry &e) {
+    if (tlb.empty()) return 0;
+    // simple pseudo-random policy: use and increment randomPtr mod tlbSize
+    unsigned idx = randomPtr % tlb.size();
+    // honor wired entries (if you later implement wired counter, skip wired slots)
+    tlb[idx] = e;
+    randomPtr = (randomPtr + 1) % tlb.size();
+    return idx;
+}
+
+int MMU::probeTLB(uint32_t vpn) const {
+    for (unsigned i = 0; i < tlb.size(); ++i) {
+        if (!tlb[i].valid) continue;
+        if (tlb[i].vpn == vpn) return static_cast<int>(i);
+    }
+    return -1;
+}
+
 void MMU::flushAll() {
     for (auto &e : tlb) e.valid = false;
 }
*** End Patch

*** Begin Patch: src/emulator.cpp
*** Update File
@@
 void Emulator::init() {
     std::cout << "Speedracer Emulator — SGI Octane1 prototype\n";
-    // register simple console device at a small range
-    // bus.register_device(...)
+    // --- MMU / CP0 wiring (new) ---
+    // Ensure cpu has a pointer to the Memory object and an MMU instance.
+    cpu.memory = &mem;
+    // create an MMU with a small TLB (64 entries default). Emulator owns the MMU.
+    cpu.mmu = new MMU(&mem, &cpu.cp0, 64);
+
+    // Insert a simple identity TLB entry for low memory (so boot ROM / startup code runs).
+    // This is a pragmatic short-circuit while MMU/TLB refill code is implemented.
+    {
+        TLBEntry e{};
+        e.valid = true;
+        e.vpn = 0x0;          // VPN 0 -> low virtual pages map to low physical pages
+        e.pfn = 0x0;          // PFN 0
+        e.dirty = true;
+        e.validP = true;
+        cpu.mmu->insertTLBEntry(0, e);
+    }
+
+    // register simple console device at a small range
+    // bus.register_device(...)
 }
*** End Patch

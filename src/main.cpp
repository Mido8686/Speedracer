// speedracer/src/main.cpp
// Simple bootstrap for Speedracer Emulator (SGI Octane1 / IP30)
// - Loads ROM file roms/ip30prom.rev4.9.bin
// - Maps it conceptually at 0xBFC00000 (informational only here)
// - Prints file size and a hex+ASCII dump of first bytes
// - Instantiates a tiny UART stub and demonstrates a write to it
//
// Build: g++ -std=c++17 -O2 -I. -o speedracer_main main.cpp
// Run:   ./speedracer_main
//
// Note: This is an initial bootstrap skeleton. Replace ROMRegion and UARTStub
// with your more complete implementations in src/devices/ when ready.

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <iomanip>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

// Default ROM path relative to repository root
static constexpr const char *DEFAULT_ROM_REL_PATH = "roms/ip30prom.rev4.9.bin";
// Suggested virtual mapping base for PROM (informational)
static constexpr uint64_t PROM_VIRT_BASE = 0xbfc00000ULL;

// Simple ROM region loader (read-only)
class ROMRegion {
public:
    ROMRegion(uint64_t base_addr = PROM_VIRT_BASE) : base(base_addr) {}
    bool load_from_file(const fs::path &p) {
        std::ifstream f(p, std::ios::binary | std::ios::ate);
        if (!f) return false;
        auto size = f.tellg();
        f.seekg(0, std::ios::beg);
        data.resize(static_cast<size_t>(size));
        if (!f.read(reinterpret_cast<char*>(data.data()), size)) return false;
        path = p;
        return true;
    }
    size_t size() const { return data.size(); }
    uint64_t base_addr() const { return base; }
    const std::vector<uint8_t>& raw() const { return data; }

    // Read a big-endian 32-bit word from the ROM (safe bounds-checked)
    uint32_t read32_be(size_t offset) const {
        if (offset + 4 > data.size()) return 0xffffffffu;
        uint32_t w = (uint32_t(data[offset]) << 24) |
                     (uint32_t(data[offset + 1]) << 16) |
                     (uint32_t(data[offset + 2]) << 8) |
                     (uint32_t(data[offset + 3]));
        return w;
    }

private:
    uint64_t base = PROM_VIRT_BASE;
    std::vector<uint8_t> data;
    fs::path path;
};

// Tiny UART MMIO stub (placeholder)
// Register layout (example):
//   0x00 - DATA (read/write, low 8 bits)
//   0x04 - STATUS
class UARTStub {
public:
    UARTStub() = default;

    // MMIO-like 32-bit write
    void write32(uint32_t offset, uint32_t value) {
        if (offset == 0x00) {
            uint8_t ch = value & 0xff;
            std::cout << static_cast<char>(ch) << std::flush; // send to host stdout
        } else if (offset == 0x04) {
            // status write (ignored in simple stub)
            status = value;
        } else {
            // unknown register; ignore for now
        }
    }

    // MMIO-like 32-bit read
    uint32_t read32(uint32_t offset) {
        if (offset == 0x00) {
            return 0; // no RX in this stub
        } else if (offset == 0x04) {
            return status;
        }
        return 0xffffffff;
    }

private:
    uint32_t status = 0x01; // TX ready by default
};

// Print a hex + ASCII dump of a buffer (like hexdump -C)
void print_hexdump(const std::vector<uint8_t> &buf, size_t max_bytes = 256) {
    size_t len = std::min(max_bytes, buf.size());
    for (size_t off = 0; off < len; off += 16) {
        std::cout << std::hex << std::setw(8) << std::setfill('0') << off << ": ";
        // hex bytes
        for (size_t i = 0; i < 16; ++i) {
            if (off + i < len) {
                std::cout << std::setw(2) << static_cast<int>(buf[off + i]) << ' ';
            } else {
                std::cout << "   ";
            }
        }
        std::cout << " ";
        // ASCII
        std::cout << std::dec << " ";
        for (size_t i = 0; i < 16; ++i) {
            if (off + i < len) {
                uint8_t c = buf[off + i];
                if (c >= 32 && c <= 126) std::cout << static_cast<char>(c);
                else std::cout << '.';
            } else {
                std::cout << ' ';
            }
        }
        std::cout << "\n";
    }
}

// Helper: try to locate ROM using current working directory or env var SPEEDRACER_ROOT
fs::path resolve_rom_path() {
    // If environment variable points to project root, prefer it
    if (const char *root = std::getenv("SPEEDRACER_ROOT")) {
        fs::path candidate = fs::path(root) / DEFAULT_ROM_REL_PATH;
        if (fs::exists(candidate)) return candidate;
    }

    // Common: run from repo root
    fs::path candidate = fs::current_path() / DEFAULT_ROM_REL_PATH;
    if (fs::exists(candidate)) return candidate;

    // Try next to executable (./roms/)
    fs::path candidate2 = fs::current_path().parent_path() / DEFAULT_ROM_REL_PATH;
    if (fs::exists(candidate2)) return candidate2;

    // As last resort, return the relative default (may not exist)
    return fs::path(DEFAULT_ROM_REL_PATH);
}

int main(int argc, char **argv) {
    std::cout << "Speedracer Emulator - bootstrap loader\n";
    std::cout << "--------------------------------------\n";

    fs::path rom_path;
    if (argc >= 2) {
        rom_path = fs::path(argv[1]);
        std::cout << "Using ROM from CLI: " << rom_path << "\n";
    } else {
        rom_path = resolve_rom_path();
        std::cout << "Resolved ROM path: " << rom_path << "\n";
    }

    if (!fs::exists(rom_path)) {
        std::cerr << "\nERROR: ROM file not found.\n"
                  << "Please place 'ip30prom.rev4.9.bin' into the roms/ directory, or\n"
                  << "run this program with the ROM path as the first argument:\n\n"
                  << "  ./speedracer_main roms/ip30prom.rev4.9.bin\n\n";
        return 1;
    }

    ROMRegion rom;
    if (!rom.load_from_file(rom_path)) {
        std::cerr << "Failed to load ROM from " << rom_path << "\n";
        return 2;
    }

    std::cout << "Loaded ROM: " << rom_path << " (" << rom.size() << " bytes)\n";
    std::cout << "Intended PROM virtual base: 0x" << std::hex << PROM_VIRT_BASE << std::dec << "\n\n";

    // Dump the first 256 bytes for verification
    std::cout << "Hex dump (first 256 bytes):\n";
    print_hexdump(rom.raw(), 256);
    std::cout << "\n";

    // Instantiate a UART stub and demonstrate writing a message
    UARTStub uart;
    std::cout << "UART stub demo -> output should follow immediately: ";
    std::string demo = "Hello from Speedracer UART stub!\n";
    for (unsigned char c : demo) {
        uart.write32(0x00, c); // data register at offset 0x00
    }

    std::cout << "\nBootstrap complete. Next steps:\n";
    std::cout << " - Replace ROMRegion/UARTStub with full src/devices implementations.\n";
    std::cout << " - Implement CPU core that sets PC to 0xBFC00000 and fetches instructions\n";
    std::cout << " - Hook UART MMIO into the CPU's memory map so PROM writes are visible\n";
    std::cout << "\nGood luck — say the word and I can generate the next step (CPU skeleton / fetch loop).\n";

    return 0;
}

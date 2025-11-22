// -----------------------------------------------------------
// tui_single_part2.cpp
// -----------------------------------------------------------
// Racer Emulator (SGI Octane1)
// Text-based UI launcher with configurable RAM
// -----------------------------------------------------------

#include <iostream>
#include <string>
#include <cstdlib>
#include "../src/emulator.h"

static uint64_t selected_ram_mb = 512;   // default 512 MB
static std::string prom_path = "ip30prom.rev4.9.bin";

// -----------------------------------------------------------
// TUI: Settings Menu
// -----------------------------------------------------------
void menu_settings()
{
    while (true)
    {
        std::cout << "\nSettings\n";
        std::cout << "--------------------------\n";
        std::cout << "Memory Size (RAM):\n";
        std::cout << "  1) 128 MB\n";
        std::cout << "  2) 256 MB\n";
        std::cout << "  3) 512 MB  (default)\n";
        std::cout << "  4) 1024 MB (1 GB)\n";
        std::cout << "  5) Back\n";
        std::cout << "Select option: ";

        int ch;
        std::cin >> ch;

        switch (ch)
        {
        case 1: selected_ram_mb = 128;  break;
        case 2: selected_ram_mb = 256;  break;
        case 3: selected_ram_mb = 512;  break;
        case 4: selected_ram_mb = 1024; break;
        case 5: return;

        default:
            std::cout << "Invalid choice.\n";
            continue;
        }

        std::cout << "\n[Settings] RAM set to "
                  << selected_ram_mb << " MB\n";
    }
}

// -----------------------------------------------------------
// Launch Emulator
// -----------------------------------------------------------
void launch_emulator()
{
    Emulator emu;

    uint64_t ram_bytes = selected_ram_mb * 1024ULL * 1024ULL;

    if (!emu.init(ram_bytes))
    {
        std::cerr << "[TUI] Emulator failed to initialize.\n";
        return;
    }

    if (!emu.load_prom(prom_path))
    {
        std::cerr << "[TUI] Failed to load PROM.\n";
        return;
    }

    std::cout << "\n[TUI] Launching SGI Octane...\n";
    std::cout << "[TUI] RAM: " << selected_ram_mb << " MB\n";

    // Run unlimited cycles (or a big number for now)
    // PROM loops forever anyway.
    emu.run(200000000); 
}

// -----------------------------------------------------------
// Main Menu
// -----------------------------------------------------------
int main()
{
    while (true)
    {
        std::cout << "\nRacer Emulator (SGI Octane1)\n";
        std::cout << "---------------------------------\n";
        std::cout << "1) Launch SGI Octane\n";
        std::cout << "2) Settings\n";
        std::cout << "3) Exit\n";
        std::cout << "Select option: ";

        int ch;
        std::cin >> ch;

        switch (ch)
        {
        case 1: launch_emulator(); break;
        case 2: menu_settings(); break;
        case 3: return 0;

        default:
            std::cout << "Invalid choice.\n";
            continue;
        }
    }

    return 0;
}

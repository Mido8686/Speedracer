// tui_single_part2.cpp
// Speedracer (SGI Octane1) TUI Launcher - Part 2
// ----------------------------------------------
// Real TUI frontend that directly launches the emulator core
// with real PROM and optional IRIX ISO.
//
// Requires:
//   - ip30prom.rev4.9.bin in current directory
//   - Optional: irix_6.5.30_1of3.iso for installation
//
// Build: g++ -O2 -std=c++17 tui_single_part2.cpp -o tui_speedracer
// (Link with your emulator objects if building together.)

#include <iostream>
#include <string>
#include <filesystem>
#include <limits>
#include <thread>
#include <chrono>

// Declare emulator entrypoint (must exist in your src/main.cpp)
int emulator_main(const std::string& prom_path, const std::string& irix_iso_path);

namespace fs = std::filesystem;

static const std::string PROM_FILENAME = "ip30prom.rev4.9.bin";
static std::string irix_iso_path = "";

void clear_screen() {
#ifdef _WIN32
    system("cls");
#else
    std::cout << "\033[2J\033[H";
#endif
}

void pause_for_key() {
    std::cout << "\nPress ENTER to continue...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void draw_header() {
    std::cout << "┌────────────────────────────────────┐\n";
    std::cout << "│  Speedracer SGI Octane1 Emulator   │\n";
    std::cout << "├────────────────────────────────────┤\n";
}

bool check_prom() {
    if (!fs::exists(PROM_FILENAME)) {
        std::cout << "❌ PROM not found: " << PROM_FILENAME << "\n";
        std::cout << "   Please copy the real PROM image to this folder.\n";
        pause_for_key();
        return false;
    }
    return true;
}

void launch_sgi_octane() {
    clear_screen();
    draw_header();

    if (!check_prom()) return;

    std::cout << "PROM found.\n";
    std::cout << "Booting SGI Octane with PROM: " << PROM_FILENAME << "\n";
    if (!irix_iso_path.empty())
        std::cout << "IRIX ISO mounted: " << irix_iso_path << "\n";
    else
        std::cout << "(No IRIX image mounted — limited boot)\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(800));

    // Call the real emulator
    int result = emulator_main(PROM_FILENAME, irix_iso_path);

    std::cout << "\n[Emulator exited with code " << result << "]\n";
    pause_for_key();
}

void install_irix() {
    clear_screen();
    draw_header();

    std::cout << "Enter IRIX ISO path (e.g., /path/to/IRIX_6.5.30_1of3.iso):\n";
    std::cout << "> ";
    std::getline(std::cin, irix_iso_path);

    if (!fs::exists(irix_iso_path)) {
        std::cout << "❌ File not found.\n";
        irix_iso_path.clear();
        pause_for_key();
        return;
    }

    std::cout << "✅ ISO found: " << irix_iso_path << "\n";
    std::cout << "You can now launch SGI Octane and install IRIX.\n";
    pause_for_key();
}

void show_settings() {
    clear_screen();
    draw_header();
    std::cout << "Settings:\n";
    std::cout << "----------------------------------------\n";
    std::cout << " Screen Resolution : 1280 × 1024\n";
    std::cout << " Refresh Rate      : 60 Hz\n";
    std::cout << " PROM File         : " << PROM_FILENAME << "\n";
    if (!irix_iso_path.empty())
        std::cout << " IRIX ISO          : " << irix_iso_path << "\n";
    else
        std::cout << " IRIX ISO          : (not set)\n";
    std::cout << "----------------------------------------\n";
    pause_for_key();
}

int main() {
    while (true) {
        clear_screen();
        draw_header();
        std::cout << "│ 1. Launch SGI Octane               │\n";
        std::cout << "│ 2. Install IRIX (mount ISO)        │\n";
        std::cout << "│ 3. Settings                        │\n";
        std::cout << "│ 4. Exit                            │\n";
        std::cout << "└────────────────────────────────────┘\n";
        std::cout << "\nSelect option: ";
        std::string choice;
        std::getline(std::cin, choice);
        if (choice == "1") {
            launch_sgi_octane();
        } else if (choice == "2") {
            install_irix();
        } else if (choice == "3") {
            show_settings();
        } else if (choice == "4" || choice == "q" || choice == "Q") {
            clear_screen();
            std::cout << "Goodbye.\n";
            break;
        } else {
            std::cout << "Invalid choice.\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(800));
        }
    }
    return 0;
}

#ifndef NES_H
#define NES_H

#include <chrono>
#include <thread>
#include <cstdlib>
#include <ctime>

#include "Bus.h"
#include "ROM.h"
#include "CPU.cpp"
class NES {
public:
    // Public member variables
    Bus bus;
    CPU& cpu = *bus.cpu;
    NESROM rom{};
    bool on = false;
    bool rom_loaded = false;
    bool A_changed = false;
    int count = 0;
    bool paused = false;

    uint8_t framebuffer[256 * 240]{};  // 8-bit color indices
    uint32_t rgbFramebuffer[256 * 240]{}; // 32-bit color for SDL

    uint32_t nesPalette[64] = {
        0x666666, 0x002A88, 0x1412A7, 0x3B00A4, 0x5C007E, 0x6E0040, 0x6C0600, 0x561D00,
        0x333500, 0x0B4800, 0x005200, 0x004F08, 0x00404D, 0x000000, 0x000000, 0x000000,
        0x6B6B6B, 0x0B3B95, 0x241CA7, 0x3B10A4, 0x631F84, 0x6D2061, 0x6F3011, 0x562C1A,
        0x344000, 0x0F5500, 0x006100, 0x006148, 0x005459, 0x002B42, 0x2F2F2F, 0x111111,
        0xA9A9A9, 0x023C9C, 0x2449CC, 0x3E40CF, 0x6B6C99, 0x7F77AA, 0x8B95C2, 0x8C8A7F,
        0xFF00A0, 0xAA0D42, 0x8C1A4E, 0x801D53, 0x922C6F, 0x9E4A6E, 0x92515D, 0x774E53,
        0x0F77BB, 0x0B9DE8, 0x2F67E0, 0x6A7FFF, 0xA2B9F1, 0x9CC6DB, 0x70A5E9, 0x5C82C7,
        0x080F99, 0x13D1F6, 0x35C8FD, 0x7F8F9E, 0xC8E0F5, 0xF3FBFF, 0xC8EBFF, 0x7F9FF7
    };

    // Public member functions
    void load_rom(const char *filename);
    void initNES();
    void run();
    void cycle();
    void end();

    uint32_t* getFramebuffer();
    void RandomizeFramebuffer();

};

#endif // NES_H
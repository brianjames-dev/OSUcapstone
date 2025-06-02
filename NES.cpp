#include "NES.h"


void NES::load_rom(const char *filename) {
    if (on == false) {
        rom.load(filename);
        rom_loaded = true;
        bus.connectROM(rom);

        // Write CHR ROM to PPU pattern table memory
        for (int i = 0; i < 1024 * 8; i++) {
            bus.ppu.writePatternTable(i, rom.chrRom[i]);
        }
        bus.ppu.decodePatternTable();

        // Write PRG ROM to CPU memory via Bus
        if (rom.mirrored) {
            // NROM-128: 16KB mirrored at 0x8000–0xBFFF and 0xC000–0xFFFF
            for (int i = 0; i < 1024 * 16; i++) {
                uint8_t byte = rom.prgRom[i];
                bus.write(0x8000 + i, byte);  // Primary bank
                bus.write(0xC000 + i, byte);  // Mirrored bank
            }
        } else {
            // NROM-256: 32KB mapped once from 0x8000–0xFFFF
            for (int i = 0; i < 1024 * 32; i++) {
                bus.write(0x8000 + i, rom.prgRom[i]);
            }
        }
    }
}

void NES::initNES() {
    std::cout << "initNES() started\n";
    if (on == true) {
        std::cout << "NES already on, returning early\n";
        return;
    }

    std::cout << "Connecting CPU to Bus...\n";
    bus.cpu = &cpu;
    cpu.connectBus(&bus);

    std::cout << "Calling cpu.reset()\n";
    cpu.reset();

    on = true;
    std::cout << "initNES() finished\n";
}

void NES::run() {
    while (on) {
        //cpu.PC = 0xC000;
        int counter = 0;
        for (int i = 0;i < 10000; i++) {
            uint8_t opcode = bus.read(cpu.PC);
            printf("Opcode: %02X\n", opcode);
            printf("counter %d \n", counter);
            cpu.printRegisters();
            cpu.execute();


            uint8_t test_passed = bus.read(0x002);
            printf("test_passed 0x%02X\n\n", test_passed);
            counter++;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void NES::cycle() {
    if (!on) return;

    // Target PPU cycles per NES frame (341 × 262 = ~89342)
    const int targetCycles = 89342;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < targetCycles; i++) {
        bus.clock();  // This will automatically call PPU/APU/CPU as needed
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;

    constexpr double ms_per_frame = 1000.0 / 60.0;  // ~16.67 ms per frame

    if (elapsed.count() < ms_per_frame) {
        SDL_Delay(static_cast<Uint32>(ms_per_frame - elapsed.count()));
    }
}

void NES::end() {
    on = false;
}


uint32_t* NES::getFramebuffer() {
    // for (int i = 0; i < 256 * 240; i++) {
    //     rgbFramebuffer[i] = nesPalette[i % 64];
    //     uint8_t colorIndex = framebuffer[i];  // Get NES color index
    //     rgbFramebuffer[i] = 0xFF000000 | nesPalette[colorIndex % 64];  // Convert to 32-bit ARGB
    // }
    return bus.ppu.rgbFramebuffer;
}

void NES::RandomizeFramebuffer() {
    for (int i = 0; i < 256 * 240; i++) {
        uint8_t r = rand() % 256;
        uint8_t g = rand() % 256;
        uint8_t b = rand() % 256;

        // Set the pixel in framebuffer as a 32-bit ARGB value
        framebuffer[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
    }
}
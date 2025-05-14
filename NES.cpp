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
    if(on == true) {
        // Uncomment to test NES at full speed, might need to add more code if system is running too fast.
          double fps = 1./60.;
          auto start = std::chrono::high_resolution_clock::now();
          int count = 0;
          while (true) {
              if (!(count % 2 == 0)) {
                  bus.clock();
              }
              count++;
              auto end = std::chrono::high_resolution_clock::now();
              std::chrono::duration<double> elapsed_time = end - start;
              std::chrono::duration<double> frame_time(fps);
              if ((elapsed_time) > frame_time) {
                  bus.cpuClockCounter = 0;
                  break;
              }
          }
        //bus.clock();
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
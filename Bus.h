#ifndef BUS_H
#define BUS_H

#include <array>
#include <cstdint>
#include "PPU.h"
#include "ROM.h"
#include "APU.h"

class CPU;
class APU;

class Bus {
public:
    Bus();  // Constructor
    ~Bus(); // Destructor

    // Devices
    CPU* cpu;
    APU* apu;
    PPU  ppu;
    std::array<uint8_t, 2 * 1024> cpuRam{}; // 2KB of CPU RAM
    NESROM* rom;


    union controller {
        struct {
            uint8_t a: 1;
            uint8_t b: 1;
            uint8_t select: 1;
            uint8_t start: 1;
            uint8_t up: 1;
            uint8_t down: 1;
            uint8_t left: 1;
            uint8_t right: 1;
        }; uint8_t reg;
    } controller1;
    controller copyController;
    int controller_read = 0;

    // Bus read and write functions
    void write(uint16_t address, uint8_t data);
    uint8_t read(uint16_t address);

    void reset();
    void clock();

    // Connect Game Rom to Bus
    void connectROM(NESROM& ROM);

    uint32_t clockCounter = 0;
    uint32_t cpuClockCounter = 0;

    // Fallback RAM for testing without ROM
    uint8_t testFallbackRAM[0x10000]{};
private:
    // Device status

    bool DMATransfer = false;
    // DMA transfers need to start on an even clock cycle
    bool DMACanStart = false;
    uint8_t DMAPage = 0x00;
    uint8_t DMAAddress = 0x00;
    uint8_t DMAData = 0x00;

};

#endif // BUS_H

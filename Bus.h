#ifndef BUS_H
#define BUS_H

#include <array>
#include <cstdint>
#include "PPU.h"
#include "ROM.h"
#include "APU.h"

class CPU;

class Bus {
public:
    Bus();  // Constructor
    ~Bus(); // Destructor

    // Devices
    CPU* cpu;
    APU* apu;
    PPU  ppu;
    std::array<uint8_t, 2 * 1024> cpuRam{};
    NESROM* rom;

    // Bus read and write functions
    void write(uint16_t address, uint8_t data);
    uint8_t read(uint16_t address);

    // Reset function
    void reset() const;
    // Clock function
    void clock();
    // Connect Game Rom to Bus
    void connectROM(NESROM& ROM);

    uint32_t clockCounter = 0;
    uint32_t cpuClockCounter = 0;

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

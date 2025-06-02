#include "Bus.h"
#include "CPU.h"
#include <thread>
#include <iostream>

Bus::Bus() {
    cpu = new CPU();
    apu = new APU();
    cpu->connectBus(this);  // Connect CPU to Bus
    apu->connectBus(this);
}

Bus::~Bus() {
    delete cpu;
    delete apu;
}

void Bus::write(uint16_t address, uint8_t data) {
    // Handles CPU RAM --> 0x0000-0x1FFF (mirrored every 0x0800)
    if (address <= 0x1FFF) {
        cpuRam[address & 0x07FF] = data;
        return;
    }

    // Handles PPU registers --> 0x2000-0x3FFF (mirrored every 8 bytes)
    if (address >= 0x2000 && address <= 0x3FFF) {
        ppu.cpuWrite(address & 0x0007, data);
        return;
    }

    // Handles APU registers --> 0x4000-0x4013, 0x4015, 0x4017
    if ((address >= 0x4000 && address <= 0x4013) || address == 0x4015 || address == 0x4017) {
        apu->writeRegister(address, data);
        return;
    }

    // Handles OAM DMA --> 0x4014
    if (address == 0x4014) {
        DMATransfer = true;
        DMAPage = data;
        DMAAddress = 0x00;
        return;
    }

    // Handles controller ports (placeholder)
    if (address == 0x4016 || address == 0x4017) {
        // TODO: Implement controller writing
        return;
    }

    // If ROM is connected, handle cartridge space writes
    if (rom && address >= 0x4020 && address <= 0xFFFF) {
        bool handled = false;

        // NROM-128 or NROM-256
        if (rom->ROMheader.flags6 == 0x00 || rom->ROMheader.flags6 == 0x01) {
            uint16_t offset;
            if (rom->mirrored && address >= 0xC000) {
                offset = address - 0xC000;
                if (offset < 0x4000) {
                    rom->prgRom[offset] = data;
                    handled = true;
                }
            } else if (!rom->mirrored && address >= 0x8000) {
                offset = address - 0x8000;
                if (offset < rom->ROMheader.prgRomSize * 16 * 1024) {
                    rom->prgRom[offset] = data;
                    handled = true;
                }
            }
        }

        if (!handled) {
            static int warnCount = 0;
            if (warnCount++ < 10) {
                std::cerr << "Warning: Ignored write to PRG-ROM at 0x" << std::hex << address << "\n";
            } else if (warnCount == 10) {
                std::cerr << "(Further PRG-ROM write warnings suppressed...)\n";
            }
        }

        return;
    }

    // Default fallback (always works for tests)
    testFallbackRAM[address] = data;
}


uint8_t Bus::read(uint16_t address) {
    if (rom == nullptr) {
        std::cerr << "ERROR: Bus::read() called before ROM is connected! Address: 0x"
                  << std::hex << address << "\n";
    }

    // Handles CPU RAM --> 0x0000-0x1FFF
    if (address <= 0x1FFF) {
        return cpuRam[address & 0x07FF];
    }

    // Handles PPU registers --> 0x2000-0x3FFF
    if (address >= 0x2000 && address <= 0x3FFF) {
        return ppu.cpuRead(address & 0x0007);
    }

    // Handles APU registers --> 0x4000–0x4017
    if ((address >= 0x4000 && address <= 0x4013) || address == 0x4015 || address == 0x4017) {
        return apu->readRegister(address);
    }

    // Handles OAM DMA
    if (address == 0x4014) {
        return 0;
    }

    // Controller reading
    if (address == 0x4016) {
        if (controller_read == 8) {
            copyController = controller1;
            controller_read = 0;
        }
        uint8_t data = copyController.reg & 1;
        copyController.reg >>= 1;
        controller_read++;
        return data;
    }

    // Cartridge memory space
    if (address >= 0x4020 && address <= 0xFFFF && rom) {
        return rom->readMemoryPRG(address);
    }

    std::cerr << "Fallback test RAM used at 0x" << std::hex << address
          << " = 0x" << std::hex << int(testFallbackRAM[address]) << "\n";
    return testFallbackRAM[address];
}


void Bus::reset() {
    cpu->reset();
    apu->reset();
    ppu.reset();
    clockCounter = 0;
    cpuClockCounter = 0;
    DMATransfer = false;
    DMACanStart = false;
    DMAPage = 0x00;
    DMAAddress = 0x00;
    DMAData = 0x00;
}

void Bus::clock() {
    // Cycle ppu & apu every clock cycle
    ppu.clock();
    apu->clock();

    // if (apu->dmc_irq_flag && !cpu->getFlag(CPU::FLAGS::I)) {
    //     cpu->irq_interrupt();
    // }

    // CPU is three times slower than ppu
    if (clockCounter % 3 == 0) {

        // Check if a DMA transfer is happening, it suspends the CPU
        if (DMATransfer) {
            if (!DMACanStart) {
                if (clockCounter % 2 == 1) {
                    DMACanStart = true;
                }
            }
            else {
                // Read from the CPU bus on even clock cycles
                if (clockCounter % 2 == 0) {
                    DMAData = read(DMAPage << 8 | DMAAddress);
                }
                // Write to PPU OAM memory on odd clock cycles
                else {
                    ppu.OAMDATA[DMAAddress] = DMAData;
                    DMAAddress++;

                    // After transfering 256 bytes end the transfer
                    if (DMAAddress == 0x00) {
                        DMATransfer = false;
                        DMACanStart = false;
                    }
                }
            }
        }
        // If no DMA transfer, cycle CPU
        else {
            cpu->cycleExecute();
            cpuClockCounter++;
        }

    }

    // if vblank started, inform cpu through nmi interrupt.
    if (ppu.nmi) {
        ppu.nmi = false;
        cpu->nmi_interrupt();
    }

    clockCounter++;
}

void Bus::connectROM(NESROM& ROM) {
    std::cout << "Bus::connectROM() called — assigning rom pointer!\n";
    ppu.connectROM(ROM);
    rom = &ROM;
}

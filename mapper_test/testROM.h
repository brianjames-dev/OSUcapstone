#ifndef NESROM_H
#define NESROM_H

#include <iostream>
#include <fstream>
#include <cstdint>
#include <string>
#include <vector>	// for std::vector prgBanks

// NES ROM header size
const size_t NES_HEADER_SIZE = 16;

// NES ROM header structure
struct NESHeader {
    char header[4];        // Should be "NES\x1A"
    uint8_t prgRomSize;    // Size of PRG ROM in 16KB units
    uint8_t chrRomSize;    // Size of CHR ROM in 8KB units
    uint8_t flags6;        // Flags 6
    uint8_t flags7;        // Flags 7
    uint8_t prgRamSize;    // Size of PRG RAM in 8KB units (0 means 8KB)
    uint8_t flags9;        // Flags 9
    uint8_t flags10;       // Flags 10 (unofficial)
    uint8_t padding[5];    // Padding, should be zero
};

// define mapper types
enum MapperType {
   	NROM = 0,
    UNROM = 2,
    // add more as needed
};

class NESROM {
public:
    // Constructor / Destructor
    NESROM() : mapperType(NROM), curBank(0), prgBanks(), prgRom(nullptr), chrRom(nullptr),
               readPRG(nullptr), writePRG(nullptr) {}
    ~NESROM();

    // public members
    uint8_t* prgRom; // For simple NROM
    uint8_t* chrRom;
    NESHeader ROMheader;
    bool mirrored = false;

    // Mapper related
    MapperType mapperType;
    uint8_t curBank; 				// for bank switching
    std::vector<uint8_t*> prgBanks; // resizeable array for UNROM

    // function pointers for read/write
    // allows the bus to always call the same function
	// with the proper mapper-specific version being used based on the currently loaded ROM
    uint8_t (NESROM::*readPRG)(uint16_t);
    void (NESROM::*writePRG)(uint16_t, uint8_t);

    // core functions
    bool load(const std::string& filepath);
    bool detect_mapper(const NESHeader& header, std::ifstream& file);
    uint8_t readMemoryPRG(uint16_t address);
    uint8_t readMemoryCHR(uint16_t address);				// unnecessary for now?
    void writeMemoryPRG(uint16_t address, uint8_t value);
    bool isValidHeader(const NESHeader& header);
    void printHeaderInfo(const NESHeader& header);
    void switchBank(uint8_t bankNumber);

private:
    // mapper-specific implementations
    uint8_t readPRG_NROM(uint16_t address);
    void writePRG_NROM(uint16_t address, uint8_t value);
    uint8_t readPRG_UNROM(uint16_t address);
    void writePRG_UNROM(uint16_t address, uint8_t value);
};

#endif // NESROM_H

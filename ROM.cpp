#include <iostream>
#include <fstream>
#include <cstdint>
#include "ROM.h"

    
// determine the type of mapper
void NESROM::detect_mapper(const NESHeader& header, std::ifstream &file) {
    if (isValidHeader(header)) {
    	switch (header.flags6) {
    	case 00:					// Mapper 0 (NROM)
    	case 01: {
    		// Calculate sizes based on the header
       		size_t prgRomSize = header.prgRomSize * 16 * 1024;		// 2 = NROM-256, mapped into $8000-$FFFF
       																// 1 = NROM-128, mapped into $8000-$BFFF AND $C000-$FFFF
        	size_t chrRomSize = header.chrRomSize * 8 * 1024;

        	if (header.prgRomSize == 1) mirrored = true;			// let the calling program know to mirror the memory

        	// Dynamically allocate memory for PRG ROM and CHR ROM
        	prgRom = new uint8_t[prgRomSize];
        	file.read(reinterpret_cast<char*>(prgRom), prgRomSize);


        	chrRom = new uint8_t[chrRomSize];
            file.read(reinterpret_cast<char*>(chrRom), chrRomSize);

            break;
        }

        case 20:					// Mapper 2 (UNROM)
        case 21: {
         	// UNROM specific stuff goes here (requires bank switching)
        	break;
        }

        // more mapper cases to follow
        }
    }
}

// function to load ROM
bool NESROM::load(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return false;
    }

    // Read the header
    NESHeader header{};
    file.read(reinterpret_cast<char*>(&header), NES_HEADER_SIZE);
    if (!isValidHeader(header)) {
        std::cerr << "Invalid NES file: " << filepath << std::endl;
        return false;
    }
    ROMheader = header;

	detect_mapper(header, file);

    // Close the file
    file.close();
    std::cout << "Successfully loaded NES ROM: " << filepath << std::endl;
    return true;
}

uint8_t NESROM::readMemoryPRG(const uint16_t address) {
    if (ROMheader.prgRomSize == 0 || prgRom == nullptr)
        return 0;

    uint16_t mappedAddress = 0;

    if (mirrored) {
        // NROM-128 (16KB mirrored at both $8000–$BFFF and $C000–$FFFF)
        mappedAddress = address & 0x3FFF;  // mask to 16KB range
    } else {
        // NROM-256 (32KB)
        mappedAddress = address - 0x8000;
    }

    if (mappedAddress < (ROMheader.prgRomSize * 16 * 1024)) {
        return prgRom[mappedAddress];
    } else {
        std::cerr << "Invalid PRG ROM read at address: 0x" << std::hex << address << std::dec << "\n";
        return 0;
    }
}

uint8_t NESROM::readMemoryCHR(uint16_t address) {
    return chrRom[address];
}

bool NESROM::isValidHeader(const NESHeader& header) {
    // Check for "NES\x1A" header
    return header.header[0] == 'N' &&
           header.header[1] == 'E' &&
           header.header[2] == 'S' &&
           header.header[3] == 0x1A;
}

void NESROM::printHeaderInfo(const NESHeader& header) {
    std::cout << "NES ROM Header Information:" << std::endl;
    std::cout << "  PRG ROM Size: " << static_cast<int>(header.prgRomSize) << " x 16KB" << std::endl;
    std::cout << "  CHR ROM Size: " << static_cast<int>(header.chrRomSize) << " x 8KB" << std::endl;
    std::cout << "  Flags6: " << std::hex << static_cast<int>(header.flags6) << std::dec << std::endl;
    std::cout << "  Flags7: " << std::hex << static_cast<int>(header.flags7) << std::dec << std::endl;
}

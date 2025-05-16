#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>		// for memcpy
#include "ROM.h"


// Destructor to clean up allocated memory
NESROM::~NESROM() {
    if (prgRom) delete[] prgRom;
    if (chrRom) delete[] chrRom;
    for (auto bank : prgBanks) {
        delete[] bank;
    }
    prgBanks.clear();
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
    
    // make sure existing data is cleaned
    if (prgRom) {
    	delete[] prgRom;
    	prgRom = nullptr;
	}
	if (!prgBanks.empty()) {
		for (auto bank : prgBanks) {
		    delete[] bank;
		}
		prgBanks.clear();
	}

	bool valid_mapper = detect_mapper(header, file);

    // Close the file
    file.close();
    
    if (!valid_mapper) {
    	printHeaderInfo(ROMheader);
    	return false;
    }
    std::cout << "Successfully loaded NES ROM: " << filepath << std::endl;
    return true;
}
    
// determine the type of mapper
bool NESROM::detect_mapper(const NESHeader& header, std::ifstream &file) {
	// this should never happen, as load() checks the header before sending the file to detect_mapper()
    if (!isValidHeader(header)) return false;
    
    // reset function pointers
    readPRG = &NESROM::readPRG_NROM;
    writePRG = &NESROM::writePRG_NROM;
    //mapperType = NROM; // Default
    
	switch (header.flags6) {
	case 0x00:					// Mapper 0 (NROM)
	case 0x01: {
	
		// set up function pointers to use the correct read / write
		mapperType = NROM;
        readPRG = &NESROM::readPRG_NROM;
        writePRG = &NESROM::writePRG_NROM;
        
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
        
        return true;

        break;
    }

    case 0x20:					// Mapper 2 (UNROM)
    case 0x21: {
    
    	mapperType = UNROM;
        readPRG = &NESROM::readPRG_UNROM;
        writePRG = &NESROM::writePRG_UNROM;
        
    	// again, calculate sizes based on header
    	// this code appears to be repeated, but I don't want to move it out of the switch statement yet
    	// in case other mappers don't follow suit
     	size_t prgRomSize = header.prgRomSize * 16 * 1024;
     	prgBanks.resize(header.prgRomSize);
     	
     	// read entire PRG ROM into temporary buffer
    	uint8_t* tempPRG = new uint8_t[prgRomSize];
    	file.read(reinterpret_cast<char*>(tempPRG), prgRomSize);
    	
    	// divide into 16KB banks
    	size_t numBanks = header.prgRomSize;		// prgRomSize is in bytes, so we shouldn't use that here
		for (size_t i = 0; i < numBanks; ++i) {
		    prgBanks[i] = new uint8_t[16 * 1024];
		    memcpy(prgBanks[i], tempPRG + i * 16 * 1024, 16 * 1024);
		}
		delete[] tempPRG;
		
		if (prgBanks.empty()) {
			std::cerr << "prgBanks is empty!\n";
			return 0;
		}
		if (curBank >= prgBanks.size()) {
			std::cerr << "curBank out of range!\n";
			return 0;
		}

		// initialize to bank 0
		curBank = 0;
		
		return true;
		break;
    }
    
    default: {
   		std::cerr << "Unsupported mapper: " << static_cast<int>(header.flags6) << std::endl;
   		return false;
    	break;
    }

    // more mapper cases to follow
    }
}


// Read from PRG memory
uint8_t NESROM::readMemoryPRG(uint16_t address) {
    return (this->*readPRG)(address);
}

// Write to PRG memory
void NESROM::writeMemoryPRG(uint16_t address, uint8_t value) {
    (this->*writePRG)(address, value);
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

void NESROM::switchBank(uint8_t bankNumber) {
	if (bankNumber < prgBanks.size()) {
        curBank = bankNumber;
        std::cout << "Switched to bank: " << (int)bankNumber << "\n";
    } else {
        std::cerr << "Invalid bank switch: " << (int)bankNumber << "\n";
    }
}

// mapper-specific read/write implementations
uint8_t NESROM::readPRG_NROM(uint16_t address) {
    if (prgRom == nullptr) return 0;

    size_t prgSizeBytes = ROMheader.prgRomSize * 16 * 1024;

    uint16_t mappedAddress;
    if (mirrored) {
        // NROM-128: mirror 16KB
        mappedAddress = address & 0x3FFF; // 16KB mirror
    } else {
        // NROM-256
        mappedAddress = address - 0x8000;
    }
    if (mappedAddress < prgSizeBytes) {
        return prgRom[mappedAddress];
    }
    return 0;
}

// mapper-specific read implementation for UNROM
uint8_t NESROM::readPRG_UNROM(uint16_t address) {
	if (prgBanks.empty()) {
        std::cerr << "Error: prgBanks is empty during read at address 0x" << std::hex << address << std::endl;
        return 0;
    }
    if (curBank >= prgBanks.size()) {
        std::cerr << "Error: curBank (" << (int)curBank << ") out of range (size: " << prgBanks.size() << ")" << std::endl;
        // Reset to 0 or handle error
        curBank = 0;
    }

    if (address >= 0x8000 && address <= 0xBFFF) {
        uint16_t offset = address - 0x8000;
        return prgBanks[curBank][offset];
    } else if (address >= 0xC000 && address <= 0xFFFF) {
        uint16_t offset = address - 0xC000;
        return prgBanks.back()[offset]; // fixed bank
    }
    return 0;
}

void NESROM::writePRG_NROM(uint16_t address, uint8_t value) {
    // NROM PRG is read-only
}

void NESROM::writePRG_UNROM(uint16_t address, uint8_t value) {
    if (address >= 0x8000 && address <= 0xFFFF) {
        uint8_t bankNumber = value & 0x07; // bits 0-2
        switchBank(bankNumber);
    }
}

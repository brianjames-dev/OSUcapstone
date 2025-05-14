#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <array>
#include <iostream>
#include <iomanip>
#include "PPU.h"
#include "ROM.h"

void PPU::cpuWrite(uint16_t addr, uint8_t data) {
    switch (addr) {
        case 0x0000: // CRTL
            control.reg = data;
            break;
        case 0x0001: // MASK
            PPUMASK = data;
            break;
        case 0x0002: // STATUS
            status.reg = data;
            break;
        case 0x0003: // OAM Address
            OAMADDR = data;
            break;
        case 0x0004: // OAM Data
            OAMDATA[OAMADDR] = data;
            break;
        case 0x0005: // SCROLL
            // first write to scroll register
            if (w == 0) {
                x = data & 0x07;
                t.coarse_x = data >> 3;
                w = 1;
            }
            // second write to scroll register
            else if(w == 1) {
                t.fine_y = data & 0x07;
                t.coarse_y = data >> 3;
                w = 0;
            }
            break;
        case 0x0006: // PPU Address
            // write to high byte on first write
            if (w == 0) {
                t.vram_register = static_cast<uint16_t>((data & 0x3F) << 8) | (t.vram_register & 0x00FF);
                w = 1;
            }
            // write to low byte on second write and copy to vram
            else if(w == 1) {
                t.vram_register = (t.vram_register & 0xFF00) | data;
                v = t;
                w = 0;
            }
            break;
        case 0x0007: // PPU Data
            // Todo: increment I bit of CRTL register by 1 or 32 depending on vertical or horizontal mode
            break;
    }
}

void PPU::writePPU(uint16_t addr, uint8_t data) {
    //TODO: Write to ppu bus between 0x0000 and 0x3FFF
}

uint8_t PPU::readPPU(uint16_t addr) {
    //TODO: Read from ppu bus between 0x0000 and 0x3FFF
}

void PPU::connectROM(NESROM& ROM) {
    this->ROM = &ROM;
}

// Pattern tables ----------------------------------------------------------------------------------------------------

// modify to allow specification of table, tile, plane?
uint8_t PPU::readPatternTable(uint16_t addr) {
    return patternTables[addr];
}

void PPU::writePatternTable(uint16_t addr, uint8_t data) {
    patternTables[addr] = data;
}

// fetch a tile
void PPU::getTile(uint8_t tileIndex, uint16_t* tileData, bool table1) {
	// get the index by multiplying the tileIndex (0 - 255) by 16 (each tile is 16 bytes)
    uint16_t index = tileIndex * 16;
    if (!table1) index += 256;			// second table
    
    /* Pretty sure this implementation is actually wrong, forgot to account for each individual bit
    for (int i = 0; i < 8; i++) {
        tileData[i] = (patternTables[index + i] << 1) | (patternTables[index + i + 8] & 0x01); // Combine bit planes
    } */
    
    // let's try again:
    for (int i = 0; i < 8; ++i) {
		// combine the data from both planes
		uint8_t low = patternTables[index + i];			// plane 0 - lower bit of each pixel
		uint8_t high = patternTables[index + i + 8];	// plane 1 - higher bit of each pixel

		uint16_t combinedPixels = 0;

		// extract bits for each pixel in a row
		for (int j = 0; j < 8; j++) {
			//start at the far left, move down as j increases
		    uint8_t bit0 = (low >> (7 - j)) & 0x01;
		    uint8_t bit1 = (high >> (7 - j)) & 0x01;
		    
		    // Combine the bits together (bit1 << 1) + bit0
		    combinedPixels |= ((bit1 << 1) | bit0) << (j * 2); // Shift left for the next pixel
		}

		// Store the combined result
		tileData[i] = combinedPixels; // Each entry now represents 8 pixels (16 bits)
	}
}

void PPU::setPixel(uint8_t x, uint8_t y, uint32_t color) {
    rgbFramebuffer[y * 256 + x] = 0xFF000000 | color;
    if (complete_frame == true) {
        std::memcpy(nextFrame, rgbFramebuffer, sizeof(nextFrame));
        complete_frame = false;
    }
}

unsigned PPU::getColor(int index) {
    std::array<uint32_t, 64> nesPalette = {
        0x545454, 0x001E74, 0x0810A0, 0x300088, 0x44004C, 0x5C0020, 0x540400, 0x3C1800,
        0x202A00, 0x083A00, 0x004000, 0x003C0A, 0x003238, 0x000000, 0x000000, 0x000000,
        0x989696, 0x074C64, 0x3032EC, 0x5C1EEC, 0x8814B0, 0xA01464, 0x982220, 0x783C0A,
        0x223C00, 0x0A6600, 0x006400, 0x00583A, 0x00393B, 0x001B2A, 0x1F1F1F, 0x111111,
        0xA9A9A9, 0x023C9C, 0x2449CC, 0x3E40CF, 0x6B6C99, 0x7F77AA, 0x8B95C2, 0x8C8A7F,
        0xFF00A0, 0xAA0D42, 0x8C1A4E, 0x801D53, 0x922C6F, 0x9E4A6E, 0x92515D, 0x774E53,
        0x0F77BB, 0x0B9DE8, 0x2F67E0, 0x6A7FFF, 0xA2B9F1, 0x9CC6DB, 0x70A5E9, 0x5C82C7,
        0x080F99, 0x13D1F6, 0x35C8FD, 0x7F8F9E, 0xC8E0F5, 0xF3FBFF, 0xC8EBFF, 0x7F9FF7
    };

    return nesPalette[index];
}

// Name tables --------------------------------------------------------------------------------------------------------

uint16_t PPU::getMirroredNameTableAddress(uint16_t address) {

    // flags6_mirror_bit will be 0 when horizontally mirroring, 1 when vertically mirroring
    int flags6_mirror_bit = ROM->ROMheader.flags6 & 1;

    uint16_t modified_address;
    if (flags6_mirror_bit == 0) {
        modified_address = address & 0x07FF;
    } else {
        modified_address = address & 0x03FF;
    }
    return modified_address;
}

// Attribute tables ---------------------------------------------------------------------------------------------------

uint16_t PPU::getAttributeTableAddress() {
    int tile_x = v.coarse_x;
    int tile_y = v.coarse_y;
    uint8_t nameTableSelection = v.nametable_x << 1 | v.nametable_y;

    uint16_t nameTableBaseAddress = nameTableBaseAddresses[nameTableSelection];

    int attributeTableIndex = (tile_y / 2) * 8 + tile_x;
    uint16_t attributeTableAddress = nameTableBaseAddress + attributeTableIndex;

    return attributeTableAddress;
}

void PPU::clock() {
    // TODO: add the code for one clock cycle of the PPU
    // There should be a lot of logic to implement as the ppu is going through the scanlines.


    if (scanline < 241 && cycle < 256) {
        // Code to test PPU scanlines with random colors
        uint32_t current_color = 0xFF000000 | getColor((cycle +total_frames) % 64);
        setPixel(cycle, scanline, current_color);
    }
    // if rendering of the screen is over, enable nmi vblank
    if (scanline == 241 && cycle == 1) {
        status.vblank = 1;
        // Check control register
        if (control.vblank_nmi_enable) {
            nmi = true;
        }
    }
    cycle++;
    if (cycle >= 341) {
        cycle = 0;
        scanline++;

        if (scanline >= 261) {
            total_frames++;
            scanline = -1;
            complete_frame = true;
        }
    }
}

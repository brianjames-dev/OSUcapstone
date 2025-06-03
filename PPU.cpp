#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <array>
#include <iostream>
#include <iomanip>
#include "PPU.h"

#include <thread>
#include <unistd.h>

#include "ROM.h"

void PPU::cpuWrite(uint16_t addr, uint8_t data) {
    //printf("PPU::cpuWrite(%04x, %04x)\n", addr, data);
    switch (addr) {
        case 0x0000: // CRTL
            control.reg = data;
            t.nametable_x = control.nametable_x;
            t.nametable_y = control.nametable_y;
            break;
        case 0x0001: // MASK
            mask.reg = data;
            break;
        case 0x0002: // STATUS
            break;
        case 0x0003: // OAM Address
            OAMADDR = data;
            break;
        case 0x0004: // OAM Data
            OAMDATA[OAMADDR] = data;
            // This is maybe a fix from running PPU test roms
            OAMADDR++;
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
                writePPU(v.vram_register, data);
                v.vram_register += (control.increment_type ? 32: 1);
            break;
    }
}

uint8_t PPU::cpuRead(uint16_t address) {
    uint8_t return_data = 0x00;
    switch(address) {
        case 0x0000: // Control register, not readable
            break;
        case 0x0001: // Mask register, not readable
            break;
        case 0x0002: // Status flag, return top 3 bits of status register, plus 5 bits of previous bus transaction
            return_data = (status.reg & 0xE0) | (dataBuffer & 0x1F);
            status.vblank = 0;
            w = 0;
            break;
        case 0x0003: // OAM address, not readable
            break;
        case 0x0004: // OAM data
            return_data = OAMDATA[OAMADDR];
            break;
        case 0x0005: // Scroll register, not readable
            break;
        case 0x0006: // PPU addresss, not readable
            break;
        case 0x0007: // PPU Data
            return_data = dataBuffer;
            dataBuffer = readPPU(v.vram_register);
            if (v.vram_register >= 0x3F00) return_data = dataBuffer;
            v.vram_register += (control.increment_type ? 32: 1);
            break;


    }
    return return_data;
}

void PPU::writePPU(uint16_t addr, uint8_t data) {
    //TODO: Write to ppu bus between 0x0000 and 0x3FFF
    //printf("PPU::writePPU: addr: %04x, data: %02x\n", addr, data);
    addr &= 0x3FFF;
    if (addr >= 0x000 && addr <= 0x1FFF) {
        patternTables[addr] = data;
    }
    else if (addr >= 0x2000 && addr <= 0x3EFF) {

        addr &= 0x0FFF;
        // Vertical mirror
        if (ROM->ROMheader.flags6 == 1) {
            if (addr >= 0x0000 && addr <= 0x03FF) {
                nameTables[addr & 0x03FF] = data;
            }
            if (addr >= 0x0400 && addr <= 0x07FF) {
                nameTables[(addr & 0x03FF) + 1024] = data;
            }
            if (addr >= 0x0800 && addr <= 0x0BFF) {
                nameTables[addr & 0x03FF] = data;
            }
            if (addr >= 0x0C00 && addr <= 0x0FFF) {
                nameTables[(addr & 0x03FF) + 1024] = data;
            }
        }
        // Horizontal mirror
        if (ROM->ROMheader.flags6 == 0) {
            if (addr >= 0x0000 && addr <= 0x03FF) {
                nameTables[addr & 0x03FF] = data;
            }
            if (addr >= 0x0400 && addr <= 0x07FF) {
                nameTables[addr & 0x03FF] = data;
            }
            if (addr >= 0x0800 && addr <= 0x0BFF) {
                nameTables[(addr & 0x03FF) + 1024] = data;
            }
            if (addr >= 0x0C00 && addr <= 0x0FFF) {
                nameTables[(addr & 0x03FF) + 1024] = data;
            }
        }
    }
    else if (addr >= 0x3F00 && addr <= 0x3FFF) {
        addr &= 0x001F;
        if (addr == 0x0010) addr = 0x0000;
        if (addr == 0x0014) addr = 0x0004;
        if (addr == 0x0018) addr = 0x0008;
        if (addr == 0x001C) addr = 0x000C;
        paletteMemory[addr] = data;
    }
}

uint8_t PPU::readPPU(uint16_t addr) {
    //TODO: Read from ppu bus between 0x0000 and 0x3FFF
    uint8_t data = 0x00;
    addr &= 0x3FFF;
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        data = patternTables[addr];
        return data;
    }
    else if (addr >= 0x2000 && addr <= 0x3EFF) {
        addr &= 0x0FFF;
        // Vertical mirror
        if (ROM->ROMheader.flags6 == 1) {
            if (addr >= 0x0000 && addr <= 0x03FF) {
                data = nameTables[addr & 0x03FF];
            }
            if (addr >= 0x0400 && addr <= 0x07FF) {
                data = nameTables[(addr & 0x03FF) + 0x0400];
            }
            if (addr >= 0x0800 && addr <= 0x0BFF) {
                data = nameTables[addr & 0x03FF];
            }
            if (addr >= 0x0C00 && addr <= 0x0FFF) {
                data = nameTables[(addr & 0x03FF) + 0x0400];
            }
        }
        // Horizontal mirror
        if (ROM->ROMheader.flags6 == 0) {
            if (addr >= 0x0000 && addr <= 0x03FF) {
                data = nameTables[addr & 0x03FF];
            }
            if (addr >= 0x0400 && addr <= 0x07FF) {
                data = nameTables[addr & 0x03FF];
            }
            if (addr >= 0x0800 && addr <= 0x0BFF) {
                data = nameTables[(addr & 0x03FF) + 0x0400];
            }
            if (addr >= 0x0C00 && addr <= 0x0FFF) {
                data = nameTables[(addr & 0x03FF) + 0x0400];
            }
        }
        return data;
    }
    else if (addr >= 0x3F00 && addr <= 0x3FFF) {
        addr &= 0x001F;
        if (addr == 0x0010) addr = 0x0000;
        if (addr == 0x0014) addr = 0x0004;
        if (addr == 0x0018) addr = 0x0008;
        if (addr == 0x001C) addr = 0x000C;
        return paletteMemory[addr] & (mask.grayscale ? 0x30: 0x3F);
    }
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

void PPU::printPatternTable() {
    size_t tableSize = 60;
    for(size_t i = 0; i < tableSize; i++) {
        printf("%04lu: %02x\n", i, patternTables[i]);
    }
}

void PPU::printPaletteMemory() {
    for(size_t i = 0; i < 32; i++) {
        printf("%02lu: %02x\n", i, paletteMemory[i]);
    }
}

// fetch a tile
void PPU::getTile(uint8_t tileIndex, uint8_t* tileData, bool table1) {
	// get the index by multiplying the tileIndex (0 - 255) by 16 (each tile is 16 bytes)
    uint16_t index = tileIndex * 16;
    if (table1 == false) {
        index += 4096;			// second table
    }

    /* Pretty sure this implementation is actually wrong, forgot to account for each individual bit
    for (int i = 0; i < 8; i++) {
        tileData[i] = (patternTables[index + i] << 1) | (patternTables[index + i + 8] & 0x01); // Combine bit planes
    } */

    // let's try again:
    for (int i = 0; i < 8; ++i) {
        // Get the low and high bit planes for this row
        //printf("pattern Table location: %d \n", index+i);
        uint8_t low = patternTables[index + i];           // plane 0 - lower bit of each pixel
        uint8_t high = patternTables[index + i + 8];      // plane 1 - higher bit of each pixel
        // extract bits for each pixel in a row
        for (int j = 0; j < 8; ++j) {
            // Extract the bits from both low and high bit planes
            uint8_t bit0 = (low >> (7 - j)) & 0x01;         // Extract bit from low plane
            uint8_t bit1 = (high >> (7 - j)) & 0x01;        // Extract bit from high plane

            // Combine the bits (bit1 is the higher bit, so we shift it left by 1)
            uint8_t combinedPixel = (bit1 << 1) | bit0;      // Combine bit1 and bit0 into a 2-bit value
            // Store the combined result in the tileData array
            tileData[i * 8 + j] = combinedPixel;

            //printf("Row %d, Pixel %d: Low Bit: %d, High Bit: %d, Combined: %d\n", i, j, bit0, bit1, combinedPixel);
        }
    }
}

void PPU::decodePatternTable() {
    for (int i = 0; i < 256; i++) {
        uint8_t currentTile[64];
        getTile(i, currentTile, true);

        for (int j = 0; j < 64; j++) {
            patternTablesDecoded[i * 64 + j] = currentTile[j];
        }
    }

     for (int i = 0; i < 256; i++) {
         uint8_t currentTile[64];
         getTile(i, currentTile, false);

         for (int j = 0; j < 64; j++) {
             patternTablesDecoded[i * 64 + j + (4096 * 4)] = currentTile[j];
         }
     }
}
void PPU::printDecodedPatternTable() {

    for (int i = 0; i < 512; i++) {
        std::cout << "Tile " << i << ":" << std::endl;

        for (int j = 0; j < 64; j++) {
            std::cout << static_cast<int>(patternTablesDecoded[i * 64 + j]) << " ";
            if ((j + 1) % 8 == 0) {
                std::cout << std::endl;
            }
        }
        std::cout << std::endl;
    }
}

void printPaletteMemory(uint8_t paletteMemory[32]) {
    // Print the palette memory in a readable format
    for (int i = 0; i < 32; i++) {
        // Print the color values in hexadecimal with padding
        std::cout << "Palette " << std::setw(2) << std::setfill('0') << i
                  << ": 0x" << std::setw(2) << std::setfill('0') << std::hex << (int)paletteMemory[i]
                  << std::dec << std::endl;
    }
}

void PPU::displayPatternTableOnScreen() {
    uint8_t current_tile;
    if (cycle < 128 && scanline < 240) {
        current_tile = patternTablesDecoded[(int)((scanline) / 8) * 1024 +((scanline * 8) % 64) + (int)(cycle/ 8) * 64 + (cycle % 8)];
    }
    else {
        current_tile = 0;
    }
    //u_int8_t current_palette = readPPU(0x3F00 + (4 << 2) + current_tile) & 0x3F;
    uint32_t current_color;
    if (current_tile == 3) {
        current_color = getColor(22);
    }
    else if (current_tile == 2) {
        current_color = getColor(14);
    }
    else if (current_tile == 1) {
        current_color = getColor(32);
    }
    else {
        current_color = getColor(1);
    }
    //uint32_t current_color = getColor(current_palette);
    //printf("Current color %08x \n", current_palette);
    setPixel(cycle, scanline, current_color);
}

void PPU::displayNameTableOnScreen(uint8_t table) {
    uint8_t nameTableByte = nameTables[(table * 1024) + ((scanline / 8) * 32) + (cycle / 8)];

    uint8_t current_tile = patternTablesDecoded[(nameTableByte * 64) + ((scanline * 8) % 64) + (cycle % 8) + (control.background_pattern * 16384)];
    uint32_t current_color;

    current_color = getColor(readPPU(0x3F00 + (0 << 2) + current_tile) % 64);

    setPixel(cycle, scanline, current_color);
}


void PPU::setPixel(uint8_t x, uint8_t y, uint32_t color) {
    rgbFramebuffer[y * 256 + x] = 0xFF000000 | color;
}

unsigned PPU::getColor(int index) {
    std::array<uint32_t, 64> nesPalette = {
        0x545454, 0xB41D01, 0xA01008, 0x880030, 0x4C0044, 0x20005C, 0x000454, 0x00183C, 0x002A20, 0x003A08, 0x004000, 0x0A3C00, 0x383200, 0x000000, 0x000000, 0x000000,
        0x969698, 0x644C07, 0xEC3230, 0xEC1E5C, 0xB01488, 0x6414A0, 0x0000FF, 0x0A3C78, 0x003C22, 0x00660A, 0x006400, 0x3A5800, 0x3B3900, 0x2A1B00, 0x1F1F1F, 0x111111,
        0xA9A9A9, 0x9C3C02, 0xCC4924, 0xCF403E, 0x996C6B, 0xAA777F, 0xC2958B, 0x009EFA, 0xA000FF, 0x00EB74, 0x4E1A8C, 0x531D80, 0xF7D52B, 0x6E4A9E, 0x525192, 0x534E77,
        0xFFFFFF, 0xE89D0B, 0xE0672F, 0xFF7F6A, 0xF2B9A2, 0xDBC69C, 0x70A5E9, 0xC7825C, 0x990F08, 0xF6D113, 0xFDC835, 0x9E8F7F, 0xF5E0C8, 0xFFFBF3, 0xFFEBC8, 0xF79F7F
    };

    return 0xFF000000 | nesPalette[index];
}

void PPU::shiftLeft(uint8_t arr[], int size) {
    // Shift all elements to the left by one position
    for (int i = 0; i < size - 1; ++i) {
        arr[i] = arr[i + 1];  // Move each element to the left
    }
}

// Name tables --------------------------------------------------------------------------------------------------------

void PPU::printNameTable() {
    for (uint8_t y = 0; y < 30; y++) {
        for (uint8_t x = 0; x < 32; x++) {
            printf("%02x  ",nameTables[y * 32 + x] );
        }
        printf("\n");
    }
    printf("\n");
}
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

void PPU::reset() {
    x = 0x00;
    w = 0x00;
    dataBuffer = 0x00;
    scanline = 0;
    cycle = 0;
    status.reg = 0x00;
    mask.reg = 0x00;
    control.reg = 0x00;
    v.vram_register = 0x0000;
    t.vram_register = 0x0000;

}

void PPU::clock() {
    // Debugging tools
    if (scanline < 241 && cycle < 256) {
        //displayPatternTableOnScreen();
        //displayNameTableOnScreen(0);
        //status.sprite_zerohit = 1;
        //printNameTable();
        //printf("\n");
    }

    // Increments the coarse x in the vram during rendering.
    auto IncrementScrollX = [&]() {
        // Check if rendering is enabled
        if (mask.enable_background_rendering || mask.enable_sprite_rendering) {
            // if the coarse x is at the end of the table, wrap around and go to the next table
            if (v.coarse_x == 31) {
                v.coarse_x = 0;
                v.nametable_x = ~v.nametable_x;
            }
            // Otherwise increment
            else {
                v.coarse_x++;
            }
        }
    };

    // Increment fine y  and coarse y during rendering
    auto IncrementScrollY = [&]() {
        if (mask.enable_background_rendering || mask.enable_sprite_rendering) {
            if (v.fine_y < 7) {
                v.fine_y++;
            }
            else {
                v.fine_y = 0;
                if (v.coarse_y == 29) {
                    v.coarse_y = 0;
                    v.nametable_y = ~v.nametable_y;
                }
                else if (v.coarse_y == 31) {
                    v.coarse_y = 0;
                }
                else {
                    v.coarse_y++;
                }
            }
        }
    };
    auto loadShiftRegisters = [&]() {
        bg_shifter_tile_lo = (bg_shifter_tile_lo & 0xFF00 | (next_bg_tile_lsb));
        bg_shifter_tile_hi = (bg_shifter_tile_hi & 0xFF00 | (next_bg_tile_msb));
        bg_shifter_attribute_lo  = (bg_shifter_attribute_lo  & 0xFF00) | ((next_bg_tile_attribute & 0b01) ? 0xFF : 0x00);
        bg_shifter_attribute_hi  = (bg_shifter_attribute_hi  & 0xFF00) | ((next_bg_tile_attribute & 0b10) ? 0xFF : 0x00);
        for (int i = 0; i < 8; ++i) {
            arr[7+i] = (next_bg_tile_attribute);
        }
    };

    auto updateShifters = [&]() {
        if (mask.enable_background_rendering) {
            bg_shifter_tile_lo <<= 1;
            bg_shifter_tile_hi <<= 1;
            bg_shifter_attribute_lo <<= 1;
            bg_shifter_attribute_hi <<= 1;
            shiftLeft(arr, 16);
        }
        if (mask.enable_sprite_rendering && cycle >= 1 && cycle < 258) {
            for (int i = 0; i < numOfSprites; i++) {
                // decrement sprite until its ready to render
                if (spriteScanline[i].x > 0) {
                    spriteScanline[i].x--;
                }
                // Once its rendered, shift it out of the array
                else {
                    sprite_shifter_pattern_lo[i] <<= 1;
                    sprite_shifter_pattern_hi[i] <<= 1;
                }
            }
        }
    };

    auto TransferAddressY = [&]() {
        if (mask.enable_background_rendering || mask.enable_sprite_rendering) {
            v.nametable_y = t.nametable_y;
            v.fine_y = t.fine_y;
            v.coarse_y = t.coarse_y;
        }
    };

    // // Transfers x scrolling information from temp vram to vram
    auto TransferAddressX = [&]() {
        if (mask.enable_background_rendering || mask.enable_sprite_rendering) {
            v.nametable_x = t.nametable_x;
            v.coarse_x = t.coarse_x;
        }
    };
    if (scanline >= -1 && scanline < 240) {
        if (scanline == -1 && cycle ==1) {
            status.vblank = 0;
            status.sprite_zerohit = 0;
            status.sprite_overflow = 0;
            for (int i = 0; i < 8; i++) {
                sprite_shifter_pattern_lo[i] = 0;
                sprite_shifter_pattern_hi[i] = 0;
            }
        }
    }

    // get information to load into shift registers
    uint16_t action = ((cycle-1) % 8);
    if (scanline >= -1 && scanline <240) {
        if (cycle > 0 && cycle < 257 || cycle > 320 && cycle <337) {
            updateShifters();

            if (action == 0) {
                loadShiftRegisters();
                next_bg_tile_id = readPPU(0x2000 | (v.vram_register & 0x0FFF));

            }
            else if (action == 2) {
                next_bg_tile_attribute = readPPU(0x23C0 | (v.nametable_y << 11)
                    | (v.nametable_x << 10)
                    | ((v.coarse_y >> 2) << 3)
                    | (v.coarse_x >> 2));
                if (v.coarse_y & 0x02) next_bg_tile_attribute >>= 4;
                if (v.coarse_x & 0x02) next_bg_tile_attribute >>=2;
                next_bg_tile_attribute &=0x03;
            }
            else if (action == 4) {
                next_bg_tile_lsb = readPPU((control.background_pattern * 4096) + (next_bg_tile_id * 16) + v.fine_y);
            }
            else if(action == 6) {
                next_bg_tile_msb = readPPU((control.background_pattern * 4096) + (next_bg_tile_id * 16) + v.fine_y + 8);
            }
            else if(action == 7) {
                IncrementScrollX();
            }
        }

    }

    if (cycle == 256) {
        IncrementScrollY();
    }

    if (cycle == 257) {
        loadShiftRegisters();
        TransferAddressX();
    }

    if (scanline == -1 && cycle >= 280 && cycle < 305) {
        TransferAddressY();
    }

    // Foreground rendering --------------------------------------------------------------------------------------------

    // Find out which sprites belong on the next scan line
    if (cycle == 257 && scanline >= 0) {
        std::memset(spriteScanline, 0xFF, 8 * sizeof(ObjectAttributeMemory));
        numOfSprites = 0;

        uint8_t OAMEntry = 0;
        bSpriteZeroHitPossible = false;
        while (OAMEntry < 64 && numOfSprites < 9) {
            int16_t diff = ((int16_t)scanline - static_cast<int16_t>(OAM[OAMEntry].y));
            if (diff >= 0 && diff < (control.sprite_size ? 16 : 8)) {
                if (numOfSprites < 8) {
                    // Check if next scanline contains a sprite zero
                    if (OAMEntry == 0) {
                        bSpriteZeroHitPossible = true;
                    }
                    memcpy(&spriteScanline[numOfSprites], &OAM[OAMEntry], sizeof(ObjectAttributeMemory));
                    numOfSprites++;
                }
            }
            OAMEntry++;
        }
        status.sprite_overflow = (numOfSprites > 8);
    }

    if (cycle == 340) {
        for (uint8_t i=0; i < numOfSprites; i++ ) {
            uint8_t sprite_pattern_bits_lo, sprite_pattern_bits_hi;
            uint16_t sprite_pattern_addr_lo, sprite_pattern_addr_hi;

            // 8x8 sprite mode
            if (!control.sprite_size) {
                // If the sprite is not flipped vertically
                if (!(spriteScanline[i].attribute & 0x80)) {
                    sprite_pattern_addr_lo = (control.sprite_pattern << 12) | (spriteScanline[i].id << 4) | (scanline - spriteScanline[i].y);
                }
                // Sprite is flipped vertically
                else {
                    sprite_pattern_addr_lo = (control.sprite_pattern << 12) | (spriteScanline[i].id << 4) | (7-(scanline - spriteScanline[i].y));
                }
            }
            // 8x16 sprite mode
            else {
                // If the sprite is not flipped vertically
                if (!(spriteScanline[i].attribute & 0x80)) {
                    if (scanline - spriteScanline[i].y < 8) {
                        // Read the top half tile
                        sprite_pattern_addr_lo = ((spriteScanline[i].id & 0x01) << 12) | ((spriteScanline[i].id & 0xFE) << 4) | ((scanline - spriteScanline[i].y) & 0x07);
                    }
                    else {
                        // Read the bottom half tile
                        sprite_pattern_addr_lo = ((spriteScanline[i].id & 0x01) << 12) | (((spriteScanline[i].id & 0xFE)+1) << 4) | ((scanline - spriteScanline[i].y) & 0x07);
                    }
                }
                // Sprite is flipped vertically
                else {
                    if (scanline - spriteScanline[i].y < 8) {
                        // Read the bottom half tile
                        sprite_pattern_addr_lo = ((spriteScanline[i].id & 0x01) << 12) | (((spriteScanline[i].id & 0xFE)+1) << 4) | ((scanline - spriteScanline[i].y) & 0x07);
                    }
                    else {
                        // Read the top half tile
                        sprite_pattern_addr_lo = ((spriteScanline[i].id & 0x01) << 12) | ((spriteScanline[i].id & 0xFE) << 4) | ((scanline - spriteScanline[i].y) & 0x07);
                    }
                }
            }
            sprite_pattern_addr_hi = sprite_pattern_addr_lo + 8;
            sprite_pattern_bits_lo = readPPU(sprite_pattern_addr_lo);
            sprite_pattern_bits_hi = readPPU(sprite_pattern_addr_hi);

            if (spriteScanline[i].attribute & 0x40) {
                auto flipbyte = [](uint8_t b) {
                    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
                    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
                    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
                    return b;
                };

                // Flip sprite horizontally
                sprite_pattern_bits_lo = flipbyte(sprite_pattern_bits_lo);
                sprite_pattern_bits_hi = flipbyte(sprite_pattern_bits_hi);
            }

            sprite_shifter_pattern_lo[i] = sprite_pattern_bits_lo;
            sprite_shifter_pattern_hi[i] = sprite_pattern_bits_hi;
        }
    }



    // if rendering of the screen is over, enable nmi vblank
    if (scanline == 241 && cycle == 1) {
        status.vblank = 1;
        // Check control register
        if (control.vblank_nmi_enable) {
            nmi = true;
        }
    }
    // Background

    uint8_t bit0 = ((bg_shifter_tile_lo << x) & 0x8000) >>15;
    uint8_t bit1 = ((bg_shifter_tile_hi << x) & 0x8000) >>15;
    uint8_t combinedPixel = (bit1 << 1) | bit0;

    // Foreground
    uint8_t fg_pixel = 0x00;
    uint8_t fg_palette = 0x00;
    uint8_t fg_priority = 0x00;

    if (mask.enable_sprite_rendering) {
        bSpriteZeroBeingRendered = false;
        for (uint8_t i = 0; i < numOfSprites; i++) {
            if (spriteScanline[i].x == 0) {
                uint8_t fg_pixel_lo = (sprite_shifter_pattern_lo[i] & 0x80) > 0;
                uint8_t fg_pixel_hi = (sprite_shifter_pattern_hi[i] & 0x80) > 0;
                fg_pixel = (fg_pixel_hi << 1) | fg_pixel_lo;

                fg_palette = (spriteScanline[i].attribute & 0x03) + 0x04;
                fg_priority = (spriteScanline[i].attribute & 0x20) == 0;

                if (fg_pixel != 0) {
                    if (i == 0 ) {
                        bSpriteZeroBeingRendered = true;
                    }
                    break;
                }
            }
        }
    }

    uint8_t pixel = 0x00;
    uint8_t palette = 0x00;

    // If both are zero, both are transparent
    if (combinedPixel == 0 && fg_pixel == 0) {
        pixel = 0x00;
        palette = 0x00;
    }
    // The foreground is visible and the background is transparent
    else if (combinedPixel == 0 && fg_pixel > 0 ) {
        pixel = fg_pixel;
        palette = fg_palette;
    }
    else if (combinedPixel > 0 && fg_pixel == 0) {
        pixel = combinedPixel;
        palette = arr[0+x];
    }
    else if (combinedPixel > 0 && fg_pixel > 0 ) {
        if (fg_priority) {
            pixel = fg_pixel;
            palette = fg_palette;
        }
        else {
            pixel = combinedPixel;
            palette = arr[0+x];
        }

        if (bSpriteZeroBeingRendered && bSpriteZeroHitPossible) {
            if (mask.enable_background_rendering && mask.enable_sprite_rendering) {
                if (~(mask.render_background_left | mask.render_sprites_left)) {
                    if (cycle >= 9 && cycle < 258) {
                        status.sprite_zerohit = 1;
                    }
                }
                else {
                    if (cycle >= 1 && cycle < 258) {
                        status.sprite_zerohit = 1;
                    }
                }
            }
        }
    }

    // Set pixel to screen
    if (scanline < 241 && cycle < 256) {
        setPixel(cycle, scanline, getColor(readPPU(0x3F00 + (palette<< 2) + pixel) % 64));
    }

    // Advance cycle and scanline
    cycle++;
    if (cycle >= 341) {
        cycle = 0;
        scanline++;

        if (scanline >= 261) {
            total_frames++;
            scanline = -1;
            complete_frame = true;

            std::memcpy(nextFrame, rgbFramebuffer, sizeof(rgbFramebuffer));
        }
    }
}
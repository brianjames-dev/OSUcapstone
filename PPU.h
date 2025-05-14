#ifndef PPU_H
#define PPU_H

#include <cstdint>  // For uint8_t and uint16_t
#include <map>
#include "ROM.h"
#include <array>
#include <cstring>
class PPU {
public:
    // Internal Registers
    union vram {
        struct {
            uint16_t coarse_x: 5;
            uint16_t coarse_y: 5;
            uint16_t nametable_x: 1;
            uint16_t nametable_y: 1;
            uint16_t fine_y: 3;
            uint16_t unused_bit: 1;
        };
        uint16_t vram_register = 0x0000;
    };

    // Current VRAM Address --- 15 bits
    vram v;
    // Temporary VRAM Address --- 15 bits
    vram t;
    uint8_t x = 0x00;               // Fine X Scroll --- 3 bits
    uint8_t w = 0x00;               // First or Second write toggle --- 1 bit

    // I/O Registers
    union PPUSTATUS {
        struct {
            uint8_t unused: 5;
            uint8_t sprite_overflow: 1;
            uint8_t sprite_zerohit: 1;
            uint8_t vblank: 1;
        };
        uint8_t reg;
    } status;

    union PPUCTRL {
        struct {
            uint8_t nametable_x: 1;
            uint8_t nametable_y: 1;
            uint8_t increment_type: 1;
            uint8_t sprite_pattern: 1;
            uint8_t background_pattern: 1;
            uint8_t sprite_size: 1;
            uint8_t ppu_master: 1;
            uint8_t vblank_nmi_enable: 1;
        }; uint8_t reg;
    } control;

    union PPUMASK {
        struct {
            uint8_t grayscale: 1;
            uint8_t render_background_left: 1;
            uint8_t render_sprites_left: 1;
            uint8_t enable_background_rendering: 1;
            uint8_t enable_sprite_rendering: 1;
            uint8_t emphasize_red: 1;
            uint8_t emphasize_green: 1;
            uint8_t emphasize_blue: 1;
        };
        uint8_t reg;
    } mask;

    //uint8_t PPUCTRL = 0x00;         // Controller
    //uint8_t PPUMASK = 0x00;         // Mask
    //uint8_t PPUSTATUS = 0x00;       // Status
    uint8_t OAMADDR = 0x00;         // Sprite RAM address
    uint8_t PPUSCROLL = 0x00;       // X and Y scroll
    uint8_t PPUADDR = 0x00;         // VRAM address
    uint8_t PPUDATA = 0x00;         // VRAM data

    // Object Attribute Memory (OAM) - 64 sprites
    struct ObjectAttributeMemory {
        uint8_t y;          // Y position of a sprite
        uint8_t id;         // ID of a tile in pattern memory
        uint8_t attribute;  // Attribute flags
        uint8_t x;          // X position of a sprite
    } OAM[64]{};

    ObjectAttributeMemory spriteScanline[8];
    uint8_t numOfSprites;

    uint8_t* OAMDATA = reinterpret_cast<uint8_t *>(OAM);
    uint8_t OAMDMA = 0x00;          // Sprite DMA

    // Method for writing data to PPU registers
    void cpuWrite(uint16_t addr, uint8_t data);

    uint8_t cpuRead(uint16_t address);

    void writePPU(uint16_t addr, uint8_t data);

    uint8_t readPPU(uint16_t addr);

    // Method to connect ROM to PPU
    void connectROM(NESROM& ROM);
    // Init ROM
    NESROM* ROM{};

    // Pattern tables------------------------------------------------------------------------------------
    std::array<uint8_t, 4096 * 4> patternTables; // two pattern tables of 256 tiles each (4096 / 16)
    std::array<uint8_t, 4096 * 16> patternTablesDecoded; // two pattern tables of 256 tiles each (4096 / 16) with combined bits

    // Palette
    uint8_t paletteMemory[32];

    // Data buffer
    uint8_t dataBuffer = 0x00;

	// read and write may be modified / unused (e.x. caller specifies table + tile, function figures out what to return)
	// useful for testing?
    uint8_t readPatternTable(uint16_t addr);

    void writePatternTable(uint16_t addr, uint8_t data);

    void printPatternTable();

    void printPaletteMemory();

    void decodePatternTable();

    void printDecodedPatternTable();

    void displayPatternTableOnScreen();

    void displayNameTableOnScreen(uint8_t table);
    void shiftLeft(uint8_t arr[], int size);

    // method to get a tile, returned as an 8-byte array of pixel info (0-3)
    void getTile(uint8_t tileIndex, uint8_t* tileData, bool table1);

    void setPixel(uint8_t x, uint8_t y, uint32_t color);

    void clock();

    int16_t cycle = 0;
    int16_t scanline = 0;
    uint16_t total_frames = 1;
    bool complete_frame = false;
    bool nmi = false;

    uint8_t framebuffer[256 * 240]{};  // 8-bit color indices
    uint32_t rgbFramebuffer[256 * 240]{}; // 32-bit color for SDL
    uint32_t nextFrame[256 * 240]{};

    unsigned getColor(int);

    void printNameTable();

    // Name tables
    std::array<uint8_t, 2048> nameTables;

    std::map<uint8_t, uint16_t> nameTableBaseAddresses = {
        {0b00000000, 0x23C0},
        {0b00000001, 0x27C0},
        {0b00000010, 0x2BC0},
        {0b00000011, 0x2FC0}
    };

    // Background
    uint8_t next_bg_tile_id = 0x00;
    uint8_t next_bg_tile_attribute = 0x00;
    uint8_t next_bg_tile_lsb = 0x00;
    uint8_t next_bg_tile_msb = 0x00;

    uint16_t bg_shifter_tile_lo = 0x0000;
    uint16_t bg_shifter_tile_hi = 0x0000;
    uint16_t bg_shifter_attribute_lo = 0x0000;
    uint16_t bg_shifter_attribute_hi = 0x0000;

    uint8_t arr[16] = {0};

    // Foreground
    uint8_t sprite_shifter_pattern_lo[8];
    uint8_t sprite_shifter_pattern_hi[8];

    bool bSpriteZeroHitPossible = false;
    bool bSpriteZeroBeingRendered = false;

    // Given an address, determines mirroring scheme and returns modified address
    uint16_t getMirroredNameTableAddress(uint16_t address);

    // Uses data from PPU v register to calculate attribute table address for current tile
    uint16_t getAttributeTableAddress();

    void reset();
};

#endif // PPU_H

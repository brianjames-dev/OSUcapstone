#ifndef CPU_CPP
#define CPU_CPP

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <array>
#include <iostream>
#include <iomanip>
#include "Bus.h"

class CPU {
public:
  // Registers
  uint8_t A = 0x00;        // Accumulator
  uint8_t X = 0x00;        // X Register
  uint8_t Y = 0x00;        // Y Register
  uint8_t S = 0xFD;        // Stack Pointer, start at 0xFD
  uint16_t PC = 0x0000;    // Program Counter, read memory at 0xFFFC and 0xFFFD for start of program;
  uint8_t P = I + U;        // Status Flags Register, start with I and U
  
  int cycles = 0;          // Countdown of cycles until the next instruction

  // RAM for CPU
  std::array<uint8_t, 64 * 1024> memory{};

  // Flags
  enum FLAGS {
    C = (1 << 0),    // Carry
    Z = (1 << 1),    // Zero
    I = (1 << 2),    // Disable Interrupts
    D = (1 << 3),    // Decimal mode, not used in NES
    B = (1 << 4),    // Break
    U = (1 << 5),    // Unused
    V = (1 << 6),    // Overflow
    N = (1 << 7)     // Negative
  };

  // Struct for returning address
  struct AddressResult {
    uint16_t address;
    int cycles;
    bool additionalCycles;
  };

  void connectBus(Bus* bus) {this->bus = bus;}

  //Example read and write through the bus.
  uint8_t readBus(uint16_t address) {
    return bus->read(address);
  }

  void writeBus(uint16_t address, uint8_t value) {
    bus->write(address, value);
  }

  // Returns value at memory address
  uint8_t readMemory(const uint16_t address) const {
    if (address < memory.size()) {
      return memory[address];
    } else {
      std::cerr << "Address out of bounds: " << address << '\n';
      return -1;
    }
  }

  // Writes value to memory address
  void writeMemory(const uint16_t address, const uint8_t value) {
      if (address < memory.size()) memory[address] = value;
      else std::cerr << "Address out of bounds: " << address << '\n';
  }

  // Sets or clears a bit of the status register
  void setFlag(FLAGS flag, bool set) {
      if (set)
          P |= flag;  // Set the flag
      else
          P &= ~flag; // Clear the flag
  }

  // Gets the flag value of a bit of the status register
  uint8_t getFlag(FLAGS flag) const {
    return ((P & flag) != 0) ? 1 : 0;
  }

  // Print the CPU registers
  void printRegisters() const {
    printf("A: [%02X]\nX: [%02X]\nY: [%02X]\nPC: [%04X]\nS: [%02X]\nP: [%02X]\n",
      A, X, Y, PC, S, P);
  }

  // Print the contents of the memory
  void printMemory() const {
    constexpr size_t bytesPerLine = 16;

    for (size_t i = 0; i < memory.size(); i += bytesPerLine) {
      // Print the memory address at the start of the line
      std::cout << std::hex << std::setw(8) << std::setfill('0') << i << ": ";

      // Print the memory values
      for (size_t j = 0; j < bytesPerLine && (i + j) < memory.size(); ++j) {
        std::cout << std::setw(2) << std::setfill('0') << static_cast<int>(memory[i + j]) << " ";
      }

      std::cout << "\n";
    }
  }

  // Set the CPU registers as specified by a console reset
  void reset() {
    const uint16_t read_address = 0xFFFC;
    uint16_t lo = readMemory(read_address);
    uint16_t hi = readMemory(read_address + 1);
    PC = (hi << 8) | lo;
    S = 0xFD;
    P = 0x00;
    setFlag(I, true);
    setFlag(U, true);
  }

  // Read and execute cycles until the next instruction has ran
  void execute() {
    int ran = 1;
    while (ran) {
      ran = cycleExecute();
    }
  }

  // Execute a cycle, running an instruction if or waiting for cycles
  int cycleExecute() {
    int ran = 1;

    // Ready to run next instruction
    if (cycles == 0) {
      // Read the opcode
      uint8_t opcode = readMemory(PC++);

      // Get the address mode and instruction type from the opcode
      //std::cout << "Opcode: 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(opcode) << std::endl;
      Instruction opcodeInstr = instructionTable[opcode];
      if (opcodeInstr.operation == nullptr || opcodeInstr.addressingMode == nullptr) {
        std::cout << "Error: Invalid opcode"; 
      }

      // Find the address, cycles and additional cycles
      AddressResult res = (this->*opcodeInstr.addressingMode)();
      //std::cout << "Cycles: " << res.cycles << "\n";

      // Execute the instruction
      int instrCycles = (this->*opcodeInstr.operation)(res.address);

      // Adds cycles to counter (if necessary)
      cycles += res.cycles;
      if (res.additionalCycles) {
        cycles += instrCycles;
      }

      // Return ran
      ran = 0;
      //std::cout << "Executed!\n";
    } else {
      //std::cout << "Waiting " << cycles << "cycles\n";
    }

    cycles--;
    return ran;
  }

  // Instruction struct for storing addressingMode and operation
  struct Instruction {
    int (CPU::*operation)(uint16_t);
    AddressResult (CPU::*addressingMode)();
  };

  // Intialize instructionTable with null values
  Instruction instructionTable[256];

  void initInstructionTable() {
      for (int i = 0; i < 256; i++) {
        instructionTable[i] = {nullptr, nullptr};
  };

    // Brian's Instruction Table START ---------------------------- //
  // LDA
  instructionTable[0xA9] = {&CPU::LDA, &CPU::Immediate};
  instructionTable[0xA5] = {&CPU::LDA, &CPU::ZeroPage};
  instructionTable[0xB5] = {&CPU::LDA, &CPU::ZeroPageX};
  instructionTable[0xAD] = {&CPU::LDA, &CPU::Absolute};
  instructionTable[0xBD] = {&CPU::LDA, &CPU::AbsoluteX};
  instructionTable[0xB9] = {&CPU::LDA, &CPU::AbsoluteY};
  instructionTable[0xA1] = {&CPU::LDA, &CPU::IndirectX};
  instructionTable[0xB1] = {&CPU::LDA, &CPU::IndirectY};

  // LDX
  instructionTable[0xA2] = {&CPU::LDX, &CPU::Immediate};
  instructionTable[0xA6] = {&CPU::LDX, &CPU::ZeroPage};
  instructionTable[0xB6] = {&CPU::LDX, &CPU::ZeroPageY};
  instructionTable[0xAE] = {&CPU::LDX, &CPU::Absolute};
  instructionTable[0xBE] = {&CPU::LDX, &CPU::AbsoluteY};

  // LDY
  instructionTable[0xA0] = {&CPU::LDY, &CPU::Immediate};
  instructionTable[0xA4] = {&CPU::LDY, &CPU::ZeroPage};
  instructionTable[0xB4] = {&CPU::LDY, &CPU::ZeroPageX};
  instructionTable[0xAC] = {&CPU::LDY, &CPU::Absolute};
  instructionTable[0xBC] = {&CPU::LDY, &CPU::AbsoluteX};

  // STA
  instructionTable[0x85] = {&CPU::STA, &CPU::ZeroPage};
  instructionTable[0x95] = {&CPU::STA, &CPU::ZeroPageX};
  instructionTable[0x8D] = {&CPU::STA, &CPU::Absolute};
  instructionTable[0x9D] = {&CPU::STA, &CPU::AbsoluteX};
  instructionTable[0x99] = {&CPU::STA, &CPU::AbsoluteY};
  instructionTable[0x81] = {&CPU::STA, &CPU::IndirectX};
  instructionTable[0x91] = {&CPU::STA, &CPU::IndirectY};

  // STX
  instructionTable[0x86] = {&CPU::STX, &CPU::ZeroPage};
  instructionTable[0x96] = {&CPU::STX, &CPU::ZeroPageY};
  instructionTable[0x8E] = {&CPU::STX, &CPU::Absolute};

  // STY
  instructionTable[0x84] = {&CPU::STY, &CPU::ZeroPage};
  instructionTable[0x94] = {&CPU::STY, &CPU::ZeroPageX};
  instructionTable[0x8C] = {&CPU::STY, &CPU::Absolute};

  // TAX, TAY, TSX, TXA, TXS, TYA
  instructionTable[0xAA] = {&CPU::TAX, &CPU::Implicit};
  instructionTable[0xA8] = {&CPU::TAY, &CPU::Implicit};
  instructionTable[0xBA] = {&CPU::TSX, &CPU::Implicit};
  instructionTable[0x8A] = {&CPU::TXA, &CPU::Implicit};
  instructionTable[0x9A] = {&CPU::TXS, &CPU::Implicit};
  instructionTable[0x98] = {&CPU::TYA, &CPU::Implicit};

  // Brian Instruction Table END -------------------------------- //


  // Add instructions
  instructionTable[0x4C] = {&CPU::JMP, &CPU::Absolute};
  instructionTable[0x6C] = {&CPU::JMP, &CPU::IndirectJMP};
  instructionTable[0x20] = {&CPU::JSR, &CPU::Absolute};
  instructionTable[0x60] = {&CPU::RTS, &CPU::Implicit};
  instructionTable[0x00] = {&CPU::BRK, &CPU::Implicit};
  instructionTable[0x40] = {&CPU::RTI, &CPU::Implicit};
  instructionTable[0x48] = {&CPU::PHA, &CPU::Implicit};
  instructionTable[0x68] = {&CPU::PLA, &CPU::Implicit};
  instructionTable[0x08] = {&CPU::PHP, &CPU::Implicit};
  instructionTable[0x28] = {&CPU::PLP, &CPU::Implicit};
  instructionTable[0x58] = {&CPU::CLI, &CPU::Implicit};
  instructionTable[0x78] = {&CPU::SEI, &CPU::Implicit};
  instructionTable[0xF0] = {&CPU::BEQ, &CPU::Relative};
  instructionTable[0xD0] = {&CPU::BNE, &CPU::Relative};
  instructionTable[0x90] = {&CPU::BCC, &CPU::Relative};
  instructionTable[0xB0] = {&CPU::BCS, &CPU::Relative};
  instructionTable[0x30] = {&CPU::BMI, &CPU::Relative};
  instructionTable[0x10] = {&CPU::BPL, &CPU::Relative};
  instructionTable[0x50] = {&CPU::BVC, &CPU::Relative};
  instructionTable[0x70] = {&CPU::BVS, &CPU::Relative};
  instructionTable[0x18] = {&CPU::CLC, &CPU::Implicit};
  instructionTable[0x38] = {&CPU::SEC, &CPU::Implicit};
  instructionTable[0x0A] = {&CPU::ASL, &CPU::Accumulator};
  instructionTable[0x06] = {&CPU::ASL, &CPU::ZeroPage};
  instructionTable[0x16] = {&CPU::ASL, &CPU::ZeroPageX};
  instructionTable[0x0E] = {&CPU::ASL, &CPU::Absolute};
  instructionTable[0x1E] = {&CPU::ASL, &CPU::AbsoluteX};
  instructionTable[0x4A] = {&CPU::LSR, &CPU::Accumulator};
  instructionTable[0x46] = {&CPU::LSR, &CPU::ZeroPage};
  instructionTable[0x56] = {&CPU::LSR, &CPU::ZeroPageX};
  instructionTable[0x4E] = {&CPU::LSR, &CPU::Absolute};
  instructionTable[0x5E] = {&CPU::LSR, &CPU::AbsoluteX};
  instructionTable[0x2A] = {&CPU::ROL, &CPU::Accumulator};
  instructionTable[0x26] = {&CPU::ROL, &CPU::ZeroPage};
  instructionTable[0x36] = {&CPU::ROL, &CPU::ZeroPageX};
  instructionTable[0x2E] = {&CPU::ROL, &CPU::Absolute};
  instructionTable[0x3E] = {&CPU::ROL, &CPU::AbsoluteX};
  instructionTable[0x6A] = {&CPU::ROR, &CPU::Accumulator};
  instructionTable[0x66] = {&CPU::ROR, &CPU::ZeroPage};
  instructionTable[0x76] = {&CPU::ROR, &CPU::ZeroPageX};
  instructionTable[0x6E] = {&CPU::ROR, &CPU::Absolute};
  instructionTable[0x7E] = {&CPU::ROR, &CPU::AbsoluteX};
  instructionTable[0xC9] = {&CPU::CMP, &CPU::Immediate};
  instructionTable[0xC5] = {&CPU::CMP, &CPU::ZeroPage};
  instructionTable[0xD5] = {&CPU::CMP, &CPU::ZeroPageX};
  instructionTable[0xCD] = {&CPU::CMP, &CPU::Absolute};
  instructionTable[0xDD] = {&CPU::CMP, &CPU::AbsoluteX};
  instructionTable[0xD9] = {&CPU::CMP, &CPU::AbsoluteY};
  instructionTable[0xC1] = {&CPU::CMP, &CPU::IndirectX};
  instructionTable[0xD1] = {&CPU::CMP, &CPU::IndirectY};
  instructionTable[0xE0] = {&CPU::CPX, &CPU::Immediate};
  instructionTable[0xE4] = {&CPU::CPX, &CPU::ZeroPage};
  instructionTable[0xEC] = {&CPU::CPX, &CPU::Absolute};
  instructionTable[0xC0] = {&CPU::CPY, &CPU::Immediate};
  instructionTable[0xC4] = {&CPU::CPY, &CPU::ZeroPage};
  instructionTable[0xCC] = {&CPU::CPY, &CPU::Absolute};
  instructionTable[0xEA] = {&CPU::NOP, &CPU::Implicit};
  instructionTable[0xD8] = {&CPU::CLD, &CPU::Implicit};
  instructionTable[0xF8] = {&CPU::SED, &CPU::Implicit};
  instructionTable[0xB8] = {&CPU::CLV, &CPU::Implicit};
  instructionTable[0x69] = {&CPU::ADC, &CPU::Immediate};
  instructionTable[0x65] = {&CPU::ADC, &CPU::ZeroPage};
  instructionTable[0x75] = {&CPU::ADC, &CPU::ZeroPageX};
  instructionTable[0x6D] = {&CPU::ADC, &CPU::Absolute};
  instructionTable[0x7D] = {&CPU::ADC, &CPU::AbsoluteX};
  instructionTable[0x79] = {&CPU::ADC, &CPU::AbsoluteY};
  instructionTable[0x61] = {&CPU::ADC, &CPU::IndirectX};
  instructionTable[0x71] = {&CPU::ADC, &CPU::IndirectY};
  instructionTable[0xE9] = {&CPU::SBC, &CPU::Immediate};
  instructionTable[0xE5] = {&CPU::SBC, &CPU::ZeroPage};
  instructionTable[0xF5] = {&CPU::SBC, &CPU::ZeroPageX};
  instructionTable[0xED] = {&CPU::SBC, &CPU::Absolute};
  instructionTable[0xFD] = {&CPU::SBC, &CPU::AbsoluteX};
  instructionTable[0xF9] = {&CPU::SBC, &CPU::AbsoluteY};
  instructionTable[0xE1] = {&CPU::SBC, &CPU::IndirectX};
  instructionTable[0xF1] = {&CPU::SBC, &CPU::IndirectY};
  instructionTable[0x24] = {&CPU::BIT, &CPU::ZeroPage};
  instructionTable[0x2C] = {&CPU::BIT, &CPU::Absolute};
  instructionTable[0x29] = {&CPU::AND, &CPU::Immediate};
  instructionTable[0x25] = {&CPU::AND, &CPU::ZeroPage};
  instructionTable[0x35] = {&CPU::AND, &CPU::ZeroPageX};
  instructionTable[0x2D] = {&CPU::AND, &CPU::Absolute};
  instructionTable[0x3D] = {&CPU::AND, &CPU::AbsoluteX};
  instructionTable[0x39] = {&CPU::AND, &CPU::AbsoluteY};
  instructionTable[0x21] = {&CPU::AND, &CPU::IndirectX};
  instructionTable[0x31] = {&CPU::AND, &CPU::IndirectY};
  instructionTable[0x09] = {&CPU::ORA, &CPU::Immediate};
  instructionTable[0x05] = {&CPU::ORA, &CPU::ZeroPage};
  instructionTable[0x15] = {&CPU::ORA, &CPU::ZeroPageX};
  instructionTable[0x0D] = {&CPU::ORA, &CPU::Absolute};
  instructionTable[0x1D] = {&CPU::ORA, &CPU::AbsoluteX};
  instructionTable[0x19] = {&CPU::ORA, &CPU::AbsoluteY};
  instructionTable[0x01] = {&CPU::ORA, &CPU::IndirectX};
  instructionTable[0x11] = {&CPU::ORA, &CPU::IndirectY};
  instructionTable[0x49] = {&CPU::EOR, &CPU::Immediate};
  instructionTable[0x45] = {&CPU::EOR, &CPU::ZeroPage};
  instructionTable[0x55] = {&CPU::EOR, &CPU::ZeroPageX};
  instructionTable[0x4D] = {&CPU::EOR, &CPU::Absolute};
  instructionTable[0x5D] = {&CPU::EOR, &CPU::AbsoluteX};
  instructionTable[0x59] = {&CPU::EOR, &CPU::AbsoluteY};
  instructionTable[0x41] = {&CPU::EOR, &CPU::IndirectX};
  instructionTable[0x51] = {&CPU::EOR, &CPU::IndirectY};
  instructionTable[0xC8] = {&CPU::INY, &CPU::Implicit};
  instructionTable[0xE8] = {&CPU::INX, &CPU::Implicit};
  instructionTable[0x88] = {&CPU::DEY, &CPU::Implicit};
  instructionTable[0xCA] = {&CPU::DEX, &CPU::Implicit};
  instructionTable[0xE6] = {&CPU::INC, &CPU::ZeroPage};
  instructionTable[0xF6] = {&CPU::INC, &CPU::ZeroPageX};
  instructionTable[0xEE] = {&CPU::INC, &CPU::Absolute};
  instructionTable[0xFE] = {&CPU::INC, &CPU::AbsoluteX};
  instructionTable[0xC6] = {&CPU::DEC, &CPU::ZeroPage};
  instructionTable[0xD6] = {&CPU::DEC, &CPU::ZeroPageX};
  instructionTable[0xCE] = {&CPU::DEC, &CPU::Absolute};
  instructionTable[0xDE] = {&CPU::DEC, &CPU::AbsoluteX};

  // Unofficial Opcodes
  // SLO
  instructionTable[0x07] = {&CPU::SLO, &CPU::ZeroPage};
  instructionTable[0x17] = {&CPU::SLO, &CPU::ZeroPageX};
  instructionTable[0x03] = {&CPU::SLO, &CPU::IndirectX};
  instructionTable[0x13] = {&CPU::SLO, &CPU::IndirectY};
  instructionTable[0x0F] = {&CPU::SLO, &CPU::Absolute};
  instructionTable[0x1F] = {&CPU::SLO, &CPU::AbsoluteX};
  instructionTable[0x1B] = {&CPU::SLO, &CPU::AbsoluteY};

  // RLA
  instructionTable[0x27] = {&CPU::RLA, &CPU::ZeroPage};
  instructionTable[0x37] = {&CPU::RLA, &CPU::ZeroPageX};
  instructionTable[0x23] = {&CPU::RLA, &CPU::IndirectX};
  instructionTable[0x33] = {&CPU::RLA, &CPU::IndirectY};
  instructionTable[0x2F] = {&CPU::RLA, &CPU::Absolute};
  instructionTable[0x3F] = {&CPU::RLA, &CPU::AbsoluteX};
  instructionTable[0x3B] = {&CPU::RLA, &CPU::AbsoluteY};

  // SRE
  instructionTable[0x47] = {&CPU::SRE, &CPU::ZeroPage};
  instructionTable[0x57] = {&CPU::SRE, &CPU::ZeroPageX};
  instructionTable[0x43] = {&CPU::SRE, &CPU::IndirectX};
  instructionTable[0x53] = {&CPU::SRE, &CPU::IndirectY};
  instructionTable[0x4F] = {&CPU::SRE, &CPU::Absolute};
  instructionTable[0x5F] = {&CPU::SRE, &CPU::AbsoluteX};
  instructionTable[0x5B] = {&CPU::SRE, &CPU::AbsoluteY};

  // RRA
  instructionTable[0x67] = {&CPU::RRA, &CPU::ZeroPage};
  instructionTable[0x77] = {&CPU::RRA, &CPU::ZeroPageX};
  instructionTable[0x63] = {&CPU::RRA, &CPU::IndirectX};
  instructionTable[0x73] = {&CPU::RRA, &CPU::IndirectY};
  instructionTable[0x6F] = {&CPU::RRA, &CPU::Absolute};
  instructionTable[0x7F] = {&CPU::RRA, &CPU::AbsoluteX};
  instructionTable[0x7B] = {&CPU::RRA, &CPU::AbsoluteY};

  // SAX
  instructionTable[0x87] = {&CPU::SAX, &CPU::ZeroPage};
  instructionTable[0x97] = {&CPU::SAX, &CPU::ZeroPageY};
  instructionTable[0x83] = {&CPU::SAX, &CPU::IndirectX};
  instructionTable[0x8F] = {&CPU::SAX, &CPU::Absolute};

  // LAX
  instructionTable[0xA7] = {&CPU::LAX, &CPU::ZeroPage};
  instructionTable[0xB7] = {&CPU::LAX, &CPU::ZeroPageY};
  instructionTable[0xA3] = {&CPU::LAX, &CPU::IndirectX};
  instructionTable[0xB3] = {&CPU::LAX, &CPU::IndirectY};
  instructionTable[0xAF] = {&CPU::LAX, &CPU::Absolute};
  instructionTable[0xBF] = {&CPU::LAX, &CPU::AbsoluteY};

  // DCP
  instructionTable[0xC7] = {&CPU::DCP, &CPU::ZeroPage};
  instructionTable[0xD7] = {&CPU::DCP, &CPU::ZeroPageX};
  instructionTable[0xC3] = {&CPU::DCP, &CPU::IndirectX};
  instructionTable[0xD3] = {&CPU::DCP, &CPU::IndirectY};
  instructionTable[0xCF] = {&CPU::DCP, &CPU::Absolute};
  instructionTable[0xDF] = {&CPU::DCP, &CPU::AbsoluteX};
  instructionTable[0xDB] = {&CPU::DCP, &CPU::AbsoluteY};

  // ISC
  instructionTable[0xE7] = {&CPU::ISC, &CPU::ZeroPage};
  instructionTable[0xF7] = {&CPU::ISC, &CPU::ZeroPageX};
  instructionTable[0xE3] = {&CPU::ISC, &CPU::IndirectX};
  instructionTable[0xF3] = {&CPU::ISC, &CPU::IndirectY};
  instructionTable[0xEF] = {&CPU::ISC, &CPU::Absolute};
  instructionTable[0xFF] = {&CPU::ISC, &CPU::AbsoluteX};
  instructionTable[0xFB] = {&CPU::ISC, &CPU::AbsoluteY};

  // ANC
  instructionTable[0x0B] = {&CPU::ANC, &CPU::Immediate};
  instructionTable[0x2B] = {&CPU::ANC, &CPU::Immediate};

  // ALR
  instructionTable[0x4B] = {&CPU::ALR, &CPU::Immediate};

  // ARR
  instructionTable[0x6B] = {&CPU::ARR, &CPU::Immediate};

  // AXS
  instructionTable[0xCB] = {&CPU::AXS, &CPU::Immediate};

  // SBC Unofficial
  instructionTable[0xEB] = {&CPU::SBC, &CPU::Immediate};

  // NOP
  instructionTable[0x04] = {&CPU::NOP, &CPU::ZeroPage};
  instructionTable[0x44] = {&CPU::NOP, &CPU::ZeroPageY};
  instructionTable[0x64] = {&CPU::NOP, &CPU::ZeroPageX};
  instructionTable[0x0C] = {&CPU::NOP, &CPU::Absolute};
  instructionTable[0x14] = {&CPU::NOP, &CPU::IndirectX};
  instructionTable[0x34] = {&CPU::NOP, &CPU::IndirectX};
  instructionTable[0x54] = {&CPU::NOP, &CPU::IndirectX};
  instructionTable[0x74] = {&CPU::NOP, &CPU::IndirectX};
  instructionTable[0xD4] = {&CPU::NOP, &CPU::IndirectX};
  instructionTable[0xF4] = {&CPU::NOP, &CPU::IndirectX};
  instructionTable[0x1A] = {&CPU::NOP, &CPU::Implicit};
  instructionTable[0x3A] = {&CPU::NOP, &CPU::Implicit};
  instructionTable[0x5A] = {&CPU::NOP, &CPU::Implicit};
  instructionTable[0x7A] = {&CPU::NOP, &CPU::Implicit};
  instructionTable[0xDA] = {&CPU::NOP, &CPU::Implicit};
  instructionTable[0xFA] = {&CPU::NOP, &CPU::Implicit};
  instructionTable[0x80] = {&CPU::NOP, &CPU::IndirectX};
  }

  // --------------------------------------  Instructions
  // BRIAN INSTRUCTIONS BEGIN ------------------------------------- //
  // Helper function to update Z and N flags
  void updateZeroNegativeFlags(uint8_t value) {
    setFlag(FLAGS::Z, value == 0);
    setFlag(FLAGS::N, value & 0x80);
  }

  // Access Instructions
  // "LDA loads a memory value into the accumulator."
  int LDA(uint16_t address) {
    uint8_t value = readMemory(address);
    A = value;

    updateZeroNegativeFlags(A);
    return 0;
  }

  // "LDX loads a memory value into the X register."
  int LDX(uint16_t address) {
    uint8_t value = readMemory(address);
    X = value;

    updateZeroNegativeFlags(X);
    return 0;
  }

  // "LDY loads a memory value into the Y register."
  int LDY(uint16_t address) {
    uint8_t value = readMemory(address);
    Y = value;

    updateZeroNegativeFlags(Y);
    return 0;
  }

  // "STA stores the accumulator value into memory."
  int STA(uint16_t address) {
    writeMemory(address, A);
    return 0;
  }

  // "STX stores the X register value into memory."
  int STX(uint16_t address) {
    writeMemory(address, X);
    return 0;
  }

  // "STY stores the Y register value into memory. "
  int STY(uint16_t address) {
    writeMemory(address, Y);
    return 0;
  }

  // Transfer Instructions
  // "TAX copies the accumulator value to the X register."
  int TAX(uint16_t) {
    X = A;

    updateZeroNegativeFlags(X);
    return 0;
  }

  // "TAY copies the accumulator value to the Y register."
  int TAY(uint16_t) {
    Y = A;

    updateZeroNegativeFlags(Y);
    return 0;
  }

  // "TSX copies the stack pointer value to the X register."
  int TSX(uint16_t) {
    X = S;

    updateZeroNegativeFlags(X);
    return 0;
  }

  // "TXA copies the X register value to the accumulator."
  int TXA(uint16_t) {
    A = X;

    updateZeroNegativeFlags(A);
    return 0;
  }

  // "TXS copies the X register value to the stack pointer."
  int TXS(uint16_t) {
    S = X;
    return 0;
  }

  // "TYA copies the Y register value to the accumulator."
  int TYA(uint16_t) {
    A = Y;

    updateZeroNegativeFlags(A);
    return 0;
  }

  // Justyn's Instructions
  // Arithmetic Instructions

  // Add carry flag and value to A
  int ADC(uint16_t address) {
    uint8_t value = readMemory(address);
    uint16_t result = A + value + getFlag(CPU::FLAGS::C);

    // Set C flag if overflow
    setFlag(CPU::FLAGS::C, result > 0xFF);

    // Set Z flag if zero
    setFlag(CPU::FLAGS::Z, result == 0);

    // Set V flag if signed overflow
    uint8_t trunc_result = result & 0xFF;
    if ((trunc_result ^ A) & (trunc_result ^ value) & 0x80) {
      setFlag(CPU::FLAGS::V, true);
    } else {
      setFlag(CPU::FLAGS::V, false);
    }

    // Set N flag if negative
    setFlag(CPU::FLAGS::N, trunc_result & 0x80);

    // Update A
    A = trunc_result;
    return 0;
  }

  // Subtract value from A with carry flag
  int SBC(uint16_t address) {
    uint8_t value = readMemory(address);
    uint16_t result = A + ~value + C;

    // Set C flag if overflow
    setFlag(C, ~(result & 0x00));

    // Set Z flag if zero
    setFlag(Z, result == 0);

    // Set V flag if signed overflow
    uint8_t trunc_result = result & 0xFF;
    if ((trunc_result ^ A) & (trunc_result ^ ~value) & 0x80) {
      setFlag(V, true);
    } else {
      setFlag(V, false);
    }

    // Set N flag if negative
    setFlag(N, result & 0x80);

    // Update A
    A = trunc_result;

    return 0;
  }

  int BIT(uint16_t address) {
    uint8_t value = readMemory(address);
    uint8_t result = A & value;
    setFlag(Z, result == 0);
    setFlag(N, value & (1 << 7));
    setFlag(V, value & (1 << 6));
    return 0;
  }

  int AND(uint16_t address) {
    A = A & readMemory(address);
    setFlag(Z, A == 0x00);
    setFlag(N, A & (1 << 7));
    return 0;
  }

  int ORA(uint16_t address) {
    A = A | readMemory(address);
    setFlag(Z, A == 0x00);
    setFlag(N, A & (1 << 7));
    return 0;
  }

  int EOR(uint16_t address) {
    A = A ^ readMemory(address);
    setFlag(Z, A == 0x00);
    setFlag(N, A & (1 << 7));
    return 0;
  }

  int INY(uint16_t address) {
    if (address != 0xFFFF) {
      throw std::runtime_error("INY called without implied mode");
    }

    Y++;
    setFlag(Z, Y == 0x00);
    setFlag(N, Y & (1 << 7));
    return 0;
  }

  int INX(uint16_t address) {
    if (address != 0xFFFF) {
      throw std::runtime_error("INX called without implied mode");
    }

    X++;
    setFlag(Z, X == 0x00);
    setFlag(N, X & (1 << 7));
    return 0;
  }

  int DEY(uint16_t address) {
    if (address != 0xFFFF) {
      throw std::runtime_error("DEY called without implied mode");
    }

    Y--;
    setFlag(Z, Y == 0x00);
    setFlag(N, Y & (1 << 7));
    return 0;
  }

  int DEX(uint16_t address) {
    if (address != 0xFFFF) {
      throw std::runtime_error("DEX called without implied mode");
    }

    X--;
    setFlag(Z, X == 0x00);
    setFlag(N, X & (1 << 7));
    return 0;
  }

  int INC(uint16_t address) {
    uint8_t value = readMemory(address);
    value ++;
    writeMemory(address, value);
    setFlag(Z, value == 0x00);
    setFlag(N, value & (1 << 7));
    // Requires 2 additional cycles
    return 2;
  }

  int DEC(uint16_t address) {
    uint8_t value = readMemory(address);
    value --;
    writeMemory(address, value);
    setFlag(Z, value == 0x00);
    setFlag(N, value & (1 << 7));
    // Requires 2 additional cycles
    return 2;
  }

  // Ethan's instructions

  //Jump instructions

  // Jump to address
  int JMP(uint16_t address) {
    PC = address;
    // Absolute is 1 cycle faster
    // Indirect unchanged
    return -1;
  }

  // Jump to subroutine
  int JSR(uint16_t address) {
    PC--;
    stack_push16(PC);
    PC = address;
    // Requires 2 additional cycles
    return 2;
  }

  // Return from subroutine
  int RTS(uint16_t address) {
    if (address != 0xFFFF) {
      throw std::runtime_error("RTS called without implied mode");
    }

    uint8_t lo = stack_pop();
    uint8_t hi = stack_pop();

    PC = (hi << 8) | lo;
    PC ++;

    // Also requires 4 additional cycles
    return 4;
  }

  // Break(software IRQ)
  int BRK(uint16_t address) {
    if (address != 0xFFFF) {
      throw std::runtime_error("BRK called without implied mode");
    }

    PC++;
    PC++;
    stack_push16(PC);

    setFlag(B, true);
    stack_push(P);

    setFlag(I, true);
    setFlag(B, false);

    const uint16_t read_address = 0xFFFE;
    uint16_t lo = readMemory(read_address);
    uint16_t hi = readMemory(read_address + 1);
    PC = (hi << 8) | lo;

    // Takes 7 cycles for some reason
    return 5;
  }

  // Return from Interrupt
  int RTI(uint16_t address) {
    if (address != 0xFFFF) {
      throw std::runtime_error("RTI called without implied mode");
    }

    // Pop stack and set to flags
    uint8_t flags = stack_pop();
    P = flags;
    setFlag(B, false);
    setFlag(U, true);

    // Pop stack twice and set to PC
    uint8_t lo = stack_pop();
    uint8_t hi = stack_pop();
    PC = (hi << 8) | lo;

    // Requirse a MASSIVE 4 additional cycles
    return 4;
  }

    // Stack instructions

  // Push A register to stack
  int PHA(uint16_t address) {
    if (address != 0xFFFF) {
      throw std::runtime_error("PHA called without implied mode");
    }

    stack_push(A);
    // For some reason requires 1 additional cycle
    return 1;
  }

  // Pop stack into A register
  int PLA(uint16_t address) {
    if (address != 0xFFFF) {
      throw std::runtime_error("PLA called without implied mode");
    }

    A = stack_pop();
    setFlag(Z, A == 0);
    setFlag(N, A & (1 << 7));
    // For some reason requires 2 additional cycles
    return 2;
  }

  // Push status flags to stack
  int PHP(uint16_t address) {
    if (address != 0xFFFF) {
      throw std::runtime_error("PHP called without implied mode");
    }

    setFlag(B, true);
    setFlag(U, true);
    stack_push(P);
    setFlag(B, false);
    // Also requires 1 additional cycle
    return 1;
  }

  // Pop status flags
  int PLP(uint16_t address) {
    if (address != 0xFFFF) {
      throw std::runtime_error("PLP called without implied mode");
    }

    P = stack_pop();
    setFlag(U, true);
    setFlag(B, false);
    // Also requires 2 additional cycles
    return 2;
  }

    // Flag instructions

  // Clear Interrupt Flag
  int CLI(uint16_t address) {
    if (address != 0xFFFF) {
      throw std::runtime_error("CLI called without implied mode");
    }

    setFlag(I, false);
    return 0;
  }

  // Set Interrupt Flag
  int SEI(uint16_t address) {
    if (address != 0xFFFF) {
      throw std::runtime_error("SEI called without implied mode");
    }

    setFlag(I, true);
    return 0;
  }

  // Carter's instructions--------------------------------------------------------

  // Branch Instructions (8 count)
	// These are signed, hence int8_t instead of uint8_t
  // Return 0 cycles if not taken, 1 if taken, and 2 if
  // taken and page crossed

	// branch if Zero flag is set
	int BEQ(uint16_t address) {
    int res = 0;
		if (getFlag(Z)) {
      res++;
			int8_t value = address;

      // Add another cycle if page crossed
      if (((PC + value) & 0xFF00) != (PC & 0xFF00)) {
        res++;
      }

			PC = PC + value;
		}
    return res;
	}

	// branch if Zero flag is not set
	int BNE(uint16_t address) {
    int res = 0;
		if (!getFlag(Z)) {
      res++;
			int8_t value = address;	

      // Add another cycle if page crossed
      if (((PC + value) & 0xFF00) != (PC & 0xFF00)) {
        res++;
      }

			PC = PC + value;
		}
    return res;
	}

	// branch if Carry flag is set
	int BCS(uint16_t address) {
		int res = 0;
    if (getFlag(C)) {
      res++;
			int8_t value = address;
			
      // Add another cycle if page crossed
      if (((PC + value) & 0xFF00) != (PC & 0xFF00)) {
        res++;
      }

      PC = PC + value;
		}
    return res;
	}
	
	// branch if Carry flag is not set
	int BCC(uint16_t address) {
    int res = 0;
		if (!getFlag(C)) {
      res++;
			int8_t value = address;

      // Add another cycle if page crossed
      if (((PC + value) & 0xFF00) != (PC & 0xFF00)) {
        res++;
      }

			PC = PC + value;
		}
    return res;
	}

	// branch if Negative flag is set (Minus)
	int BMI(uint16_t address) {
    int res = 0;
		if (getFlag(N)) {
			int8_t value = address;

      // Add another cycle if page crossed
      if (((PC + value) & 0xFF00) != (PC & 0xFF00)) {
        res++;
      }

		PC = PC + value;
		}
    return res;
	}

	// branch if Negative flag is not set (Plus)
	int BPL(uint16_t address) {
    int res = 0;
		if (!getFlag(N)) {
      res++;
			int8_t value = (address);

      // Add another cycle if page crossed
      if (((PC + value) & 0xFF00) != (PC & 0xFF00)) {
        res++;
      }

			PC = PC + value;
		}
    return res;
	}

	// branch if oVerflow flag is set
	int BVS(uint16_t address) {
    int res = 0;
		if (getFlag(V)) {
      res++;
			int8_t value = address;

      // Add another cycle if page crossed
      if (((PC + value) & 0xFF00) != (PC & 0xFF00)) {
        res++;
      }

			PC = PC + value;
		}
    return res;
	}
	
	// branch if oVerflow flag is not set
	int BVC(uint16_t address) {
    int res = 0;
		if (!getFlag(V)) {
      res++;
			int8_t value = address;

      // Add another cycle if page crossed
      if (((PC + value) & 0xFF00) != (PC & 0xFF00)) {
        res++;
      }

			PC = PC + value;
		}
    return res;
	}

	// Carry Flag Instructions (2 count)
	
	//set the carry flag
	int SEC(uint16_t address) {
    if (address != 0xFFFF) {
      throw std::runtime_error("SEC called without implied mode");
    }

		setFlag(C, true);
    return 0;
	}
	
	// clear the carry flag
	int CLC(uint16_t address) {
    if (address != 0xFFFF) {
      throw std::runtime_error("CLC called without implied mode");
    }

		setFlag(C, false);
    return 0;
	}

  // Zachary's Instructions

  // Shift Instructions

  // Arithmetic Shift Left
  int ASL(uint16_t address) {
    uint8_t value;
    // Checking for accumulator mode
    if (address == 0xFFFF) {
      value = A;
    } else {
      value = readMemory(address);
    }
    // MSB = Most Significant Bit
    int value_msb = (value >> 7) & 1;
    uint8_t shifted_value = value << 1;
    int shifted_value_msb = (shifted_value >> 7) & 1;
    // C, N, Z flags are affected
    setFlag(C, value_msb);
    setFlag(N, shifted_value_msb);
    setFlag(Z, shifted_value == 0);
    if (address == 0xFFFF) {
      A = shifted_value;
    } else {
      writeMemory(address, value);
      writeMemory(address, shifted_value);
    }

    // 2 additional cycles
    return 2;
  }

  // Logical Shift Right
  int LSR(uint16_t address) {
    uint8_t value;
    if (address == 0xFFFF) {
      value = A;
    } else {
      value = readMemory(address); 
    }
    // LSB = Least Significant Bit
    int value_lsb = value & 1;
    uint8_t shifted_value = value >> 1;
    int shifted_value_msb = (shifted_value >> 7) & 1;
    setFlag(C, value_lsb);
    setFlag(N, shifted_value_msb);
    setFlag(Z, shifted_value == 0);
    if (address == 0xFFFF) {
      A = shifted_value;
    } else {
      writeMemory(address, value);
      writeMemory(address, shifted_value);
    }
    // Requires 2 additional cycles for all modes but absolute
    return 2;
  }

  // Rotate Left
  int ROL(uint16_t address) {
    uint8_t value;
    if (address == 0xFFFF) {
      value = A;
    } else {
      value = readMemory(address);
    }
    int value_msb = (value >> 7) & 1;
    uint8_t shifted_value = value << 1;
    int shifted_value_msb = (shifted_value >> 7) & 1;
    // The value held in the Carry flag is shifted into the LSB of the new value
    if (getFlag(C) == 1) {
      shifted_value |= 1;
    }
    setFlag(C, value_msb);
    setFlag(N, shifted_value_msb);
    setFlag(Z, shifted_value == 0);
    if (address == 0xFFFF) {
      A = shifted_value;
    } else {
      writeMemory(address, value);
      writeMemory(address, shifted_value);
    }
    // Requires 2 additional cycles for all modes but absolute
    return 2;
  }

  // Rotate Right
  int ROR(uint16_t address) {
    uint8_t value;
    if (address == 0xFFFF) {
      value = A;
    } else {
      value = readMemory(address);
    }
    int value_lsb = value & 1;
    uint8_t shifted_value = value >> 1;
    int shifted_value_msb = (shifted_value >> 7) & 1;
    // The value held in the Carry flag is shifted into the MSB of the new value
    if (getFlag(C) == 1) {
      shifted_value |= 0x80;
    }
    setFlag(C, value_lsb);
    setFlag(N, shifted_value_msb);
    setFlag(Z, shifted_value == 0);
    if (address == 0xFFFF) {
      A = shifted_value;
    } else {
      writeMemory(address, value);
      writeMemory(address, shifted_value);
    }
    // Also requires 2 additional cycles for all modes but absolute
    return 2;
  }

  // Compare Instructions

  // Compare to Accumulator
  int CMP(uint16_t address) {
    uint8_t value = readMemory(address);
    uint8_t result = A - value;
    int result_msb = (result >> 7) & 1;
    setFlag(C, A >= value);
    setFlag(N, result_msb);
    setFlag(Z, A == value);
    return 0;
  }

  // Compare to X Register
  int CPX(uint16_t address) {
    uint8_t value = readMemory(address);
    uint8_t result = X - value;
    int result_msb = (result >> 7) & 1;
    setFlag(C, X >= value);
    setFlag(N, result_msb);
    setFlag(Z, X == value);
    return 0;
  }

  // Compare to Y Register
  int CPY(uint16_t address) {
    uint8_t value = readMemory(address);
    uint8_t result = Y - value;
    int result_msb = (result >> 7) & 1;
    setFlag(C, Y >= value);
    setFlag(N, result_msb);
    setFlag(Z, Y == value);
    return 0;
  }

  // No Operation
  int NOP(uint16_t address) {
    if (address != 0xFFFF) {
      throw std::runtime_error("NOP called without implied mode");
    }

    return 0;
  }

  // Flag Instructions

  // Clear Decimal Flag
  int CLD(uint16_t address) {
    if (address != 0xFFFF) {
      throw std::runtime_error("CLD called without implied mode");
    }

    setFlag(D, 0);
    return 0;
  }

  // Set Decimal Flag
  int SED(uint16_t address) {
    if (address != 0xFFFF) {
      throw std::runtime_error("SED called without implied mode");
    }

    setFlag(D, 1);
    return 0;
  }

  // Clear Overflow Flag
  int CLV(uint16_t address) {
    if (address != 0xFFFF) {
      throw std::runtime_error("CLV called without implied mode");
    }

    setFlag(V, 0);
    return 0;
  }

  // --------------------------------------  Unofficial Opcodes
  // Shift Left and Or
  int SLO(uint16_t address) {
    int res = ASL(address);
    res += ORA(address);
    return res;
  }

  // Rotate Left and And
  int RLA(uint16_t address) {
    int res = ROL(address);
    res += AND(address);
    return res;
  }

  // Shift Right and Exclusive Or
  int SRE(uint16_t address) {
    int res = LSR(address);
    res += EOR(address);
    return res;
  }

  // Rotate Right and Add With Carry
  int RRA(uint16_t address) {
    int res = ROR(address);
    res += ADC(address);
    return res;
  }

  // Store A and X
  int SAX(uint16_t address) {
    uint8_t result = A & X;
    writeMemory(address, result);
    return 0;
  }

  // Load A and X
  int LAX(uint16_t address) {
    int res = LDA(address);
    res += LDX(address);
    return res;
  }

  // Decrement Memory and Compare
  int DCP(uint16_t address) {
    int res = DEC(address);
    res += CMP(address);
    return res;
  }

  // Increment Memory and Subtract with Borrow
  int ISC(uint16_t address) {
    int res = INC(address);
    res += SBC(address);
    return res;
  }

  // AND then setting NZC flags
  int ANC(uint16_t address) {
    A = A & readMemory(address);
    setFlag(Z, A == 0x00);
    setFlag(N, A & (1 << 7));
    setFlag(C, A & (1 << 7));
    return 0;
  }

  // AND then LSR A
  int ALR(uint16_t address) {
    // AND - Immediate
    A = A & readMemory(address);
    setFlag(Z, A == 0x00);
    setFlag(N, A & (1 << 7));

    // LSR - Accumulator
    uint8_t value = A;

    int value_lsb = value & 1;
    uint8_t shifted_value = value >> 1;
    int shifted_value_msb = (shifted_value >> 7) & 1;
    setFlag(C, value_lsb);
    setFlag(N, shifted_value_msb);
    setFlag(Z, shifted_value == 0);

    A = shifted_value;
    return 0;
  }

  // AND then ROR A (CV flags set differently)
  int ARR(uint16_t address) {
    // AND - Immediate
    A = A & readMemory(address);
    setFlag(Z, A == 0x00);
    setFlag(N, A & (1 << 7));

    // ROR - Accumulator
    uint8_t value = A;

    uint8_t shifted_value = value >> 1;
    // The value held in the Carry flag is shifted into the MSB of the new value
    if (getFlag(C) == 1) {
      shifted_value |= 0x80;
    }
    int shifted_value_msb = (shifted_value >> 7) & 1;
    int bit_five = (shifted_value >> 5) & 1;
    int bit_six = (shifted_value >> 6) & 1;

    setFlag(C, bit_six);
    setFlag(N, shifted_value_msb);
    setFlag(Z, shifted_value == 0);
    setFlag(V, bit_six^bit_five);

    A = shifted_value;
    return 0;
  }

  // Sets X to (A AND X) minus value without borrow & Updates NZC flags
  int AXS(uint16_t address) {
    uint8_t value = readMemory(address);
    X = (A & X) - value;

    setFlag(C, 0);
    setFlag(N, (X >> 7) & 1);
    setFlag(Z, X == 0);
    return 0;
  }

  // --------------------------------------  Addressing Modes
  // Address is implied, returning 0xFFFF as indicator
  AddressResult Implicit() {
    uint16_t address = 0xFFFF;
    int cycles = 2;
    // Set to true to allow for special cases
    bool additionalCycles = true;

    return {address, cycles, additionalCycles};
  }

  // Address is directly at the next PC
  AddressResult Immediate() {
    uint16_t address = PC++;
    int cycles = 2;
    bool additionalCycles = false;

    return {address, cycles, additionalCycles};
  }

  // Address is the accumulator, returning 0xFFFF as indicator
  // Logic to be handled in instruction
  AddressResult Accumulator() {
    uint16_t address = 0xFFFF;
    int cycles = 2;
    bool additionalCycles = false;

    return {address, cycles, additionalCycles};
  }

  // Return next PC += offset, stored in PC
  AddressResult Relative() {
    // Offset is unsigned, at the memory location stored in PC
    int8_t offset = static_cast<int8_t>(readMemory(PC));
    PC++;
    // Return a uint16_t as forced, which will be converted back into
    // an int8_t in the branch instructions
    uint16_t addr = offset & 0xFF;
    int cycles = 2;
    // Branch instructions will modify cycles based on 
    // branch taken and page crossing
    bool additionalCycles = true;

    return {addr, cycles, additionalCycles};
  }

  // Return address from zero page memory
  AddressResult ZeroPage() {
    uint16_t address = readMemory(PC++);
    int cycles = 3;
    bool additionalCycles = true;

    return {address, cycles, additionalCycles};
  }

  // Reuturn address + X from zero page memory, wrapped
  AddressResult ZeroPageX() {
    uint16_t address = readMemory(PC++) + X & 0xFF;
    int cycles = 4;
    bool additionalCycles = true;

    return {address, cycles, additionalCycles};
  }

  // Reuturn address + Y from zero page memory, wrapped
  AddressResult ZeroPageY() {
    uint16_t address = readMemory(PC++) + Y & 0xFF;
    int cycles = 4;
    bool additionalCycles = true;

    return {address, cycles, additionalCycles};
  }

  // Return a full 16 bit address from the next two PC
  AddressResult Absolute() {
    uint16_t addr = readMemory(PC) | readMemory(PC + 1) << 8;
    PC += 2;
    int cycles = 4;
    bool additionalCycles = true;

    return {addr, cycles, additionalCycles};
  }

  // Return a full 16 bit address from the next two PC + X
  AddressResult AbsoluteX() {
    uint16_t addr = readMemory(PC) | readMemory(PC + 1) << 8;
    addr += X;
    int cycles = 4;
    // additionalCycles dependent on page boundary crossed
    if ((addr & 0xFF00) != (readMemory(PC + 1) << 8)) {
      cycles++;
    }
    bool additionalCycles = true;
    PC += 2;

    return {addr, cycles, additionalCycles};
  }

  // Return a full 16 bit address from the next two PC + Y
  AddressResult AbsoluteY() {
    uint16_t addr = readMemory(PC) | readMemory(PC + 1) << 8;
    addr += Y;
    int cycles = 4;
    if ((addr & 0xFF00) != (readMemory(PC + 1) << 8)) {
      cycles++;
    }
    bool additionalCycles = true;
    PC += 2;

    return {addr, cycles, additionalCycles};
  }

  // Return an address using the operand as a pointer
  AddressResult Indirect() {
    // Find 16 bit address from operand
    uint16_t pointer = readMemory(PC) | readMemory(PC + 1) << 8;
    // Find address referenced by pointer
    uint16_t addr = readMemory(pointer) | readMemory((pointer + 1) & 0xFFFF) << 8;
    PC += 2;
    int cycles = 5;
    bool additionalCycles = false;

    return {addr, cycles, additionalCycles};
  }

  // Return a full 16 bit address from a pointer in the zero page + X
  AddressResult IndirectX() {
    uint16_t ptrAddr = (readMemory(PC++) + X) & 0xFF;
    uint16_t addr = readMemory(ptrAddr) | (readMemory(ptrAddr + 1) & 0xFF) << 8;
    int cycles = 6;
    bool additionalCycles = false;

    return {addr, cycles, additionalCycles};
  }

  // Return a full 16 bit address from a pointer in the zero page + Y
  AddressResult IndirectY() {
    uint16_t ptrAddr = readMemory(PC++);
    uint16_t addr = readMemory(ptrAddr) | (readMemory(ptrAddr + 1) & 0xFF) << 8;
    addr += Y;
    int cycles = 5;
    // Add 1 cycle if page crossed
    if ((addr & 0xFF00) != ((readMemory(ptrAddr + 1) & 0xFF) << 8)) {
      cycles++;
    }
    bool additionalCycles = true;
    return {addr, cycles, additionalCycles};
  }

  // Special Indirect mode for JMP
  AddressResult IndirectJMP() {
    uint16_t lo = readMemory(PC);
    PC ++;
    uint16_t hi = readMemory(PC);
    PC ++;

    uint16_t addr = (hi << 8) | lo;

    if (lo == 0x00FF) {
        hi = readMemory(addr & 0xFF00);
    }
    else {
        hi = readMemory(addr + 1);
    }
    lo = readMemory(addr);

    addr = (hi << 8) | lo;
    int cycles = 5;
    bool additionalCycles = false;

    return {addr, cycles, additionalCycles};
  }

  // Constructor
  CPU() {
    initInstructionTable();
  }

	// Push to the stack (8 bits)
  void stack_push(uint8_t value) {
    uint16_t stack_address = 0x0100 + S;
    writeMemory(stack_address, value);
    S -= 1;
  }

  // Push to the stack (16 bits)
  void stack_push16(uint16_t value) {
    uint8_t low_byte = value & 0xFF;
    uint8_t high_byte = (value >> 8) & 0xFF;

    uint16_t stack_address = 0x0100 + S;
    writeMemory(stack_address, high_byte);
    S -= 1;
    stack_address -= 1;
    writeMemory(stack_address, low_byte);
    S -= 1;
  }

  // Pop from the stack
  uint8_t stack_pop() {
    S += 1;
    uint16_t stack_address = 0x0100 + S;
    uint8_t stack_top_value = readMemory(stack_address);
    return stack_top_value;
  }

  // CPU Handling of an NMI Interrupt
  void nmi_interrupt() {
    stack_push16(PC);
    stack_push(P);
    setFlag(FLAGS::I, 1);
    PC = 0xFFFA;
  }

  // CPU Handling of an IRQ Interrupt
  void irq_interrupt() {
    // Check if interrupt is allowed
    if (getFlag(I) == 0) {
    // Push PC and P to stack
    stack_push16(PC);
    setFlag(B, false);
    stack_push(P);
    setFlag(I, true);
    // Get new PC location
    const uint16_t read_address = 0xFFFE;
    uint16_t lo = readMemory(read_address);
    uint16_t hi = readMemory(read_address + 1);
    PC = (hi << 8) | lo;
    }
  }

  private:
  Bus *bus = nullptr;
};
#endif

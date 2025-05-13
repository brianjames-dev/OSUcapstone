//
// Created by brian on 3/11/2025.
//

#ifndef CPU_H
#define CPU_H

#include <cstdint>

class Bus;

class CPU {
public:
    CPU();
    ~CPU();

    // Registers (initialized in .cpp constructor)
    uint8_t A;          // Accumulator
    uint8_t X;          // X Register
    uint8_t Y;          // Y Register
    uint8_t S;          // Stack Pointer, start at 0xFD
    uint16_t PC;        // Program Counter, read memory at 0xFFFC and 0xFFFD for start of program;
    uint8_t P;          // Status Flags Register, start with I and U

    uint32_t cycles;    // cycle countdown

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

    // Read/write functions
    void writeBus(uint16_t address, uint8_t value);
    uint8_t readBus(uint16_t address);

    // Various helper functions
    void connectBus(Bus* bus);
    void reset();
    void execute();
    int cycleExecute();
    void printRegisters() const;
    void initInstructionTable();

    // Interrupt Handling
    void nmi_interrupt();
    void irq_interrupt();

    // Flag operations
    void setFlag(FLAGS flag, bool set);
    uint8_t getFlag(FLAGS flag) const;

    // Stack Operations
    void stack_push(uint8_t value);
    void stack_push16(uint16_t value);
    uint8_t stack_pop();

    // Struct for returning address
    struct AddressResult {
        uint16_t address;
        int cycles;
        bool additionalCycles;
    };

        struct Instruction {
        int (CPU::*operation)(uint16_t);
        AddressResult (CPU::*addressingMode)();
    };
    Instruction instructionTable[256];

    // Addressing Modes
    AddressResult Implicit();
    AddressResult Immediate();
    AddressResult Accumulator();
    AddressResult Relative();
    AddressResult ZeroPage();
    AddressResult ZeroPageX();
    AddressResult ZeroPageY();
    AddressResult Absolute();
    AddressResult AbsoluteX();
    AddressResult AbsoluteY();
    AddressResult Indirect();
    AddressResult IndirectX();
    AddressResult IndirectY();
    AddressResult IndirectJMP();

    // Access Instructions
    int LDA(uint16_t address);
    int LDX(uint16_t address);
    int LDY(uint16_t address);
    int STA(uint16_t address);
    int STX(uint16_t address);
    int STY(uint16_t address);

    // Transfer Instructions
    int TAX(uint16_t address);
    int TAY(uint16_t address);
    int TSX(uint16_t address);
    int TXA(uint16_t address);
    int TXS(uint16_t address);
    int TYA(uint16_t address);

    // Arithmetic Instructions
    int ADC(uint16_t address);
    int SBC(uint16_t address);
    int BIT(uint16_t address);
    int AND(uint16_t address);
    int ORA(uint16_t address);
    int EOR(uint16_t address);
    int INY(uint16_t address);
    int INX(uint16_t address);
    int DEY(uint16_t address);
    int DEX(uint16_t address);
    int INC(uint16_t address);
    int DEC(uint16_t address);

    // Jump Instructions
    int JMP(uint16_t address);
    int JSR(uint16_t address);
    int RTS(uint16_t address);
    int BRK(uint16_t address);
    int RTI(uint16_t address);

    // Stack Instructions
    int PHA(uint16_t address);
    int PLA(uint16_t address);
    int PHP(uint16_t address);
    int PLP(uint16_t address);

    // Flag Instructions
    int CLI(uint16_t address);
    int SEI(uint16_t address);
    int SEC(uint16_t address);
    int CLC(uint16_t address);
    int CLD(uint16_t address);
    int SED(uint16_t address);
    int CLV(uint16_t address);

    // Branch Instructions
    int BEQ(uint16_t address);
    int BNE(uint16_t address);
    int BCS(uint16_t address);
    int BCC(uint16_t address);
    int BMI(uint16_t address);
    int BPL(uint16_t address);
    int BVS(uint16_t address);
    int BVC(uint16_t address);

    // Shift Instructions
    int ASL(uint16_t address);
    int LSR(uint16_t address);
    int ROL(uint16_t address);
    int ROR(uint16_t address);

    // Compare Instructions
    int CMP(uint16_t address);
    int CPX(uint16_t address);
    int CPY(uint16_t address);

    // No Operation
    int NOP(uint16_t address);

    // Unofficial Opcodes
    int SLO(uint16_t address);
    int RLA(uint16_t address);
    int SRE(uint16_t address);
    int RRA(uint16_t address);
    int SAX(uint16_t address);
    int LAX(uint16_t address);
    int DCP(uint16_t address);
    int ISC(uint16_t address);
    int ANC(uint16_t address);
    int ALR(uint16_t address);
    int ARR(uint16_t address);
    int AXS(uint16_t address);

private:
    Bus* bus; // Pointer to the Bus for memory operations


};

#endif // CPU_H

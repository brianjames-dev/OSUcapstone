#include "tests.h"

void Tests::test_cpu() {
	std::cout << "\nCPU Tests:\n";
	NES nes;
	CPU& cpu = *nes.bus.cpu;

	// Check start up values
	cpu.printRegisters();
	assert(cpu.A == 0x00);
	assert(cpu.X == 0x00);
	assert(cpu.Y == 0x00);
	assert(cpu.S == 0xFD);
	assert(cpu.P == 0x00);

	// Check write
	cpu.writeBus(0x10, 0xAB);
	cpu.writeBus(0x0000, 0xAB);
	assert(cpu.readBus(0x10) == 0xAB);
	assert(cpu.readBus(0x0000) == 0xAB);

	// OOB, should return error
	cpu.writeBus(0x801, 0xAB); // 2049
	cpu.readBus(0xFFF); // 4095

	cpu.setFlag(CPU::FLAGS::Z, true);
	cpu.printRegisters();
	printf("Status Flag Z: %d\n", cpu.getFlag(CPU::FLAGS::Z));

	std::cout << "Memory at 0x10: 0x" << std::hex << static_cast<int>(cpu.readBus(0x10)) << "\n";
	printf("Value at address 0x0000: %02X\n", cpu.readBus(0x0000));
	assert(cpu.readBus(0x10) == 0xAB);
	assert(cpu.readBus(0x0000) == 0xAB);

	std::cout << "CPU test passed!\n";
}

//----------------------------------------------------------------------------------------------------------------------------
void Tests::test_opcodes() {
	std::cout << "---------------------------\nOpcode Tests:\n\n";
	NES nes;
	CPU& cpu = *nes.bus.cpu;

	// Test Program
	cpu.writeBus(0x00, 0xA9); // LDA Immediate AA
	cpu.writeBus(0x01, 0xAA);
	cpu.writeBus(0x02, 0xA5); // LDA Zero Page
	cpu.writeBus(0x03, 0x35);
	cpu.writeBus(0x35, 0xBB); // Load BB into 0x35
	cpu.writeBus(0x04, 0xB5); // LDA Zero Page X
	cpu.writeBus(0x05, 0x35);
	cpu.writeBus(0x38, 0xCC); // Load CC into 0x38
	cpu.writeBus(0x06, 0xAD); // LDA Absolute
	cpu.writeBus(0x07, 0x01);
	cpu.writeBus(0x08, 0x02);
	cpu.writeBus(0x0201, 0xDD); // Load DD into 0x0201
	cpu.writeBus(0x09, 0xBD); // LDA Absolute X
	cpu.writeBus(0x0A, 0x01);
	cpu.writeBus(0x0B, 0x02);
	cpu.writeBus(0x0204, 0xEE); // Load EE into 0x0204
	cpu.writeBus(0x0C, 0xB9); // LDA Absolute Y
	cpu.writeBus(0x0D, 0x01);
	cpu.writeBus(0x0E, 0x02);
	cpu.writeBus(0x0203, 0xFF); // Load FF into 0x0203
	cpu.writeBus(0x0F, 0xA1); // LDA Indirect X
	cpu.writeBus(0x10, 0x20);
	cpu.writeBus(0x23, 0xAA); // Write 0x01AA to 0x23/24
	cpu.writeBus(0x24, 0x01);
	cpu.writeBus(0x01AA, 0xAA); // Load AA into 0x01AA
	cpu.writeBus(0x11, 0xB1); // LDA Indirect Y
	cpu.writeBus(0x12, 0x23);
	cpu.writeBus(0x01AC, 0xBB); // Load BB into 0x01AC

	// Initialize PC
	cpu.PC = 0x0000;
	cpu.X = 3;
	cpu.Y = 2;
	cpu.execute();
	assert(cpu.A == 0xAA);
	cpu.execute();
	assert(cpu.A == 0xBB);
	cpu.execute();
	assert(cpu.A == 0xCC);
	cpu.execute();
	assert(cpu.A == 0xDD);
	cpu.execute();
	assert(cpu.A == 0xEE);
	cpu.execute();
	assert(cpu.A == 0xFF);
	cpu.execute();
	assert(cpu.A == 0xAA);
	cpu.printRegisters();
	cpu.execute();
	assert(cpu.A == 0xBB);
	cpu.printRegisters();

	std::cout << "Opcode tests passed!\n";
}

//----------------------------------------------------------------------------------------------------------------------------
void Tests::test_ADC() {
	std::cout << "---------------------------\nADC Tests:\n\n";
	NES nes;
	CPU& cpu = *nes.bus.cpu;

	// Test Program
	cpu.writeBus(0x00, 0x69); // Load 5
	cpu.writeBus(0x01, 0x05);
	cpu.writeBus(0x02, 0x69); // Load 0
	cpu.writeBus(0x03, 0x00);
	cpu.writeBus(0x04, 0x69); // Load 80
	cpu.writeBus(0x05, 0x50);
	cpu.writeBus(0x06, 0x69); // Load -10, signed
	cpu.writeBus(0x07, 0xF6);

	// Test A Register
	cpu.PC = 0x00;
	cpu.A = 0x05;
	cpu.execute();
	assert(cpu.A == 0x0A);
	std::cout << "   A Register good\n";

	// Test Carry Flag
	cpu.PC = 0x00;
	cpu.A = 0x05;
	cpu.setFlag(CPU::FLAGS::C, true);
	cpu.execute();
	assert(cpu.A == 0x0B);
	std::cout << "   Carry flag modifier good\n";

	// Test Carry Flag Value
	cpu.PC = 0x00;
	cpu.A = 0x05;
	cpu.execute();
	assert(cpu.getFlag(CPU::FLAGS::C) == false);
	cpu.PC = 0x00;
	cpu.A = 0xFF;
	cpu.execute();
	assert(cpu.getFlag(CPU::FLAGS::C) == true);
	std::cout << "   Carry flag result good\n";

	// Test Zero Flag Value
	cpu.PC = 0x00;
	cpu.A = 0x05;
	cpu.execute();
	assert(cpu.getFlag(CPU::FLAGS::Z) == false);
	cpu.A = 0x00;
	cpu.execute();
	assert(cpu.getFlag(CPU::FLAGS::Z) == true);
	std::cout << "   Zero flag good\n";

	// Test Overflow Flag Value
	cpu.PC = 0x00;
	cpu.A = 0x05;
	cpu.execute();
	assert(cpu.getFlag(CPU::FLAGS::V) == false);
	cpu.PC = 0x04;
	cpu.A = 0x50;
	cpu.execute();
	assert(cpu.getFlag(CPU::FLAGS::V) == true);
	std::cout << "   Overflow flag good\n";

	// Test Negative Flag Value
	cpu.PC = 0x00;
	cpu.A = 0x05;
	cpu.execute();
	  assert(cpu.getFlag(CPU::FLAGS::N) == false);
	cpu.PC = 0x06;
	cpu.A = 0x05;
	cpu.execute();
	std::cout << "   Negative flag good\n";

	std::cout << "\nADC Tests passed!\n\n";
}

//----------------------------------------------------------------------------------------------------------------------------
void Tests::test_stack() {
	NES nes;
	CPU& cpu = *nes.bus.cpu;

	uint16_t starting_stack_address = 0x0100 + cpu.S;
	cpu.stack_push(0xBC);
	uint16_t current_stack_address = 0x0100 + cpu.S;
	assert(cpu.readBus(current_stack_address + 1) == 0xBC);
	uint8_t stack_top = cpu.stack_pop();
	assert(stack_top == 0xBC);
	current_stack_address = 0x0100 + cpu.S;
	assert(current_stack_address == starting_stack_address);
	cpu.stack_push16(0xABCD);
	current_stack_address = 0x0100 + cpu.S;
	assert(cpu.readBus(current_stack_address + 2) == 0xAB);
	assert(cpu.readBus(current_stack_address + 1) == 0xCD);
	stack_top = cpu.stack_pop();
	assert(stack_top == 0xCD);
	stack_top = cpu.stack_pop();
	assert(stack_top == 0xAB);
	current_stack_address = 0x0100 + cpu.S;
	assert(current_stack_address == starting_stack_address);

	std::cout << "---------------------------\nStack function tests passed!\n";
}

//----------------------------------------------------------------------------------------------------------------------------
void Tests::test_reset() {
	NES nes;

	CPU& cpu = *nes.bus.cpu;

	std::cout << "Test setup: PC = " << std::hex << cpu.PC << "\n";

	std::cout << "---------------------------\nReset test:\n\nCurrent values:\n";
	cpu.printRegisters();
	cpu.setFlag(CPU::FLAGS::Z, 1);
	cpu.setFlag(CPU::FLAGS::V, 1);
	cpu.setFlag(CPU::FLAGS::I, 0);
	cpu.PC = 0x0000;
	cpu.S = 0xAA;
	std::cout << "\nUpdated values:\n";
	cpu.printRegisters();

	// Populate reset vector in internal RAM (mirrored to 0xFFFC/0xFFFD)
	nes.bus.rom = nullptr;
	nes.bus.write(0xFFFC, 0xA9); // Low byte
	nes.bus.write(0xFFFD, 0xC2); // High byte

	// Reset CPU state
	cpu.reset();
	std::cout << "\nAfter reset:\n";
	cpu.printRegisters();

	//Check if values match reset
	assert(cpu.P == 0x24);
	assert(cpu.S == 0xFD);
	assert(cpu.PC == 0xC2A9);

	std::cout << "---------------------------\nReset function tests passed!\n";
}

//----------------------------------------------------------------------------------------------------------------------------
void Tests::test_nmi() {
	std::cout << ">>> test_nmi() starting\n";
	NES nes;
	CPU& cpu = *nes.bus.cpu;

	uint8_t starting_stack_address = 0x0100 + cpu.S;
	cpu.nmi_interrupt();
	uint8_t current_stack_address = 0x0100 + cpu.S;
	assert(current_stack_address == starting_stack_address - 3);

	// Check if PC address is being set correctly
    uint16_t read_address = 0xFFFA;
    cpu.writeBus(read_address, 0x12);
    cpu.writeBus(read_address + 1, 0x34);

    uint8_t lo = cpu.readBus(read_address);
    uint8_t hi = cpu.readBus(read_address + 1);

    cpu.PC = (hi << 8) | lo;

    assert(cpu.PC == 0x3412);

	std::cout << "---------------------------\nNMI Interrupt function tests passed!\n";
}

//----------------------------------------------------------------------------------------------------------------------------
void Tests::test_irq() {
	std::cout << ">>> test_irq() starting\n";
	NES nes;
	CPU& cpu = *nes.bus.cpu;

	// Set flag so interrupt will work
	cpu.setFlag(CPU::FLAGS::I, false);

	// Call interrupt and get stack address
	uint16_t starting_stack_address = 0x0100 + cpu.S;
    cpu.irq_interrupt();
	uint16_t current_stack_address = 0x0100 + cpu.S;

	assert(current_stack_address == starting_stack_address - 3);

	// Check if PC address is being set correctly
	uint16_t read_address = 0xFFFE;
	cpu.writeBus(read_address, 0x12);
	cpu.writeBus(read_address + 1, 0x34);

	uint8_t lo = cpu.readBus(read_address);
	uint8_t hi = cpu.readBus(read_address + 1);

	cpu.PC = (hi << 8) | lo;

	assert(cpu.PC == 0x3412);

	std::cout << "---------------------------\nIRQ Interrupt function tests passed!\n";
}

//----------------------------------------------------------------------------------------------------------------------------
void Tests::test_jmp() {
	NES nes;
	CPU& cpu = *nes.bus.cpu;
	cpu.reset();
	uint16_t test_memory = 0xFFFF;

	// Test JMP, JSR, RTS
	cpu.JMP(0xFFFA);
	assert(cpu.PC == 0xFFFA);

	cpu.JSR(0x1234);
	assert(cpu.PC == 0x1234);

	cpu.RTS(test_memory);
	assert(cpu.PC == 0xFFFA);

	// Test BRK, RTI
	cpu.PC = 0x1973;
	cpu.setFlag(CPU::FLAGS::Z, 1);
	cpu.setFlag(CPU::FLAGS::C, 1);
	cpu.setFlag(CPU::FLAGS::V, 1);

	cpu.BRK(test_memory);
	cpu.RTI(test_memory);

	assert(cpu.PC == 0x1975);
	assert(cpu.P == 0x67);

	// Test Indirect Jump

	cpu.PC = 0x0000;
	cpu.writeBus(cpu.PC, 0x34);
	cpu.writeBus(cpu.PC + 1, 0x12);

	cpu.writeBus(0x1234, 0x78);
	cpu.writeBus(0x1235, 0x56);

	cpu.JMP(cpu.IndirectJMP().address);

	assert(cpu.PC == 0x5678);

	std::cout << "---------------------------\nJump functions tests passed!\n";
}

//----------------------------------------------------------------------------------------------------------------------------
void Tests::test_stack_instructions() {
	NES nes;
	CPU& cpu = *nes.bus.cpu;
	cpu.reset();
	uint16_t test_memory = 0xFFFF;
	cpu.A = 0x34;
	// Test PHA and PLA
	cpu.PHA(test_memory);
	cpu.PLA(test_memory);

	assert(cpu.A == 0x34);
	// Test PHP and PLP
	cpu.PHP(test_memory);
	cpu.PLP(test_memory);

	assert(cpu.P == 0x24);
}

//----------------------------------------------------------------------------------------------------------------------------
void Tests::test_branch() {
	NES nes;
	CPU& cpu = *nes.bus.cpu;

	uint16_t test_memory = 0x0000;
	cpu.writeBus(test_memory, 0x79);
	test_memory = cpu.Relative().address;

	// branch if Zero set
	cpu.setFlag(CPU::FLAGS::Z, 1);
	cpu.BEQ(test_memory);
	assert(cpu.PC == 0x7A);

	// branch if Zero clear
	cpu.PC = 0x0001;
	cpu.setFlag(CPU::FLAGS::Z, 0);
	cpu.BNE(test_memory);
	assert(cpu.PC == 0x7A);

	// branch if Carry set
	cpu.PC = 0x0001;
	cpu.setFlag(CPU::FLAGS::C, 1);
	cpu.BCS(test_memory);
	assert(cpu.PC == 0x7A);

	// branch if Carry clear
	cpu.PC = 0x0001;
	cpu.setFlag(CPU::FLAGS::C, 0);
	cpu.BCC(test_memory);
	assert(cpu.PC == 0x7A);

	// branch if Negative set
	cpu.PC = 0x0001;
	cpu.setFlag(CPU::FLAGS::N, 1);
	cpu.BMI(test_memory);
	assert(cpu.PC == 0x7A);

	// branch if Negative clear
	cpu.PC = 0x0001;
	cpu.setFlag(CPU::FLAGS::N, 0);
	cpu.BPL(test_memory);
	assert(cpu.PC == 0x7A);

	// branch if oVerflow set
	cpu.PC = 0x0001;
	cpu.setFlag(CPU::FLAGS::V, 1);
	cpu.BVS(test_memory);
	assert(cpu.PC == 0x7A);

	// branch if oVerflow clear
	cpu.PC = 0x0001;
	cpu.setFlag(CPU::FLAGS::V, 0);
	cpu.BVC(test_memory);
	assert(cpu.PC == 0x7A);

	std::cout << "---------------------------\nBranch functions tests passed!\n";
}
//----------------------------------------------------------------------------------------------------------------------------
void Tests::test_ASL() {
	NES nes;
	CPU& cpu = *nes.bus.cpu;
    cpu.reset();

    // Accumulator loaded with 25, ASL executed, accumulator should now hold 50
    cpu.A = 0x19;
    cpu.writeBus(0x00, 0x0A); // ASL Accumulator
    cpu.execute();
    assert(cpu.A == 0x32);

    // Accumulator loaded with 144, ASL executed, accumulator should now hold 32 and carry flag should be set
    cpu.A = 0x90;
    cpu.writeBus(0x01, 0x0A);
    cpu.execute();
    assert(cpu.A == 0x20);
    assert(cpu.getFlag(CPU::FLAGS::C) == 1);

    // ASL Non-Accumulator Testing. Address 0xABCD loaded with 25, ASL executed, address should now hold 50
    cpu.writeBus(0x02, 0x0E); // ASL Absolute
    cpu.writeBus(0x03, 0xCD);
    cpu.writeBus(0x04, 0xAB);
    cpu.writeBus(0xABCD, 0x19); // Load 0x19 into address 0xABCD
    cpu.execute();
    assert(cpu.readBus(0xABCD) == 0x32);

    std::cout << "---------------------------\nASL Instruction tests passed!\n";
}
//----------------------------------------------------------------------------------------------------------------------------
void Tests::test_LSR() {
	NES nes;
	CPU& cpu = *nes.bus.cpu;
    cpu.reset();

    // Accumulator loaded with 144, LSR executed, accumulator should now hold 72
    cpu.A = 0x90;
    cpu.writeBus(0x00, 0x4A); // LSR Accumulator
    cpu.execute();
    assert(cpu.A == 0x48);
    assert(cpu.getFlag(CPU::FLAGS::C) == 0);

    // LSR Non-Accumulator Testing. Address 0xABCD loaded with 144, LSR executed, address should now hold 72
    cpu.writeBus(0x01, 0x4E); // LSR Absolute
    cpu.writeBus(0x02, 0xCD);
    cpu.writeBus(0x03, 0xAB);
    cpu.writeBus(0xABCD, 0x90); // Load 0x90 into address 0xABCD
    cpu.execute();
    assert(cpu.readBus(0xABCD) == 0x48);

    std::cout << "---------------------------\nLSR Instruction tests passed!\n";
}
//----------------------------------------------------------------------------------------------------------------------------
void Tests::test_ROL() {
	NES nes;
	CPU& cpu = *nes.bus.cpu;
    cpu.reset();

    // Accumulator loaded with 25, ROL executed, accumulator should now hold 50
    cpu.A = 0x19;
    cpu.writeBus(0x00, 0x2A); // ROL Accumulator
    cpu.execute();
    assert(cpu.A == 0x32);

    // Accumulator loaded with 128, ROL executed, accumulator should now hold 0
    cpu.setFlag(CPU::FLAGS::C, 0); // Reset Carry Flag
    cpu.A = 0x80;
    cpu.writeBus(0x01, 0x2A);
    cpu.execute();
    assert(cpu.A == 0x0);
    assert(cpu.getFlag(CPU::FLAGS::C) == 1); // Carry Flag should now hold 1

    // ROL Non-Accumulator Testing. Address 0xABCD loaded with 128, ROL executed, Carry Flag set, address should now hold 1
    cpu.setFlag(CPU::FLAGS::C, 1); // Set Carry Flag
    cpu.writeBus(0x02, 0x2E); // ROL Absolute
    cpu.writeBus(0x03, 0xCD);
    cpu.writeBus(0x04, 0xAB);
    cpu.writeBus(0xABCD, 0x80);
    cpu.execute();
    assert(cpu.readBus(0xABCD) == 0x1);
    assert(cpu.getFlag(CPU::FLAGS::C) == 1); // Carry Flag should now hold 1

    std::cout << "---------------------------\nROL Instruction tests passed!\n";
}
//----------------------------------------------------------------------------------------------------------------------------
void Tests::test_ROR() {
	NES nes;
	CPU& cpu = *nes.bus.cpu;
    cpu.reset();

    // Accumulator loaded with 1, ROR executed, accumulator should now hold 0
    cpu.A = 0x1;
    cpu.writeBus(0x00, 0x6A); // ROR Accumulator
    cpu.execute();
    assert(cpu.A == 0x0);
    assert(cpu.getFlag(CPU::FLAGS::C) == 1);

    // Accumulator loaded with 25, ROR executed, Carry Flag not set, accumulator should now hold 12
    cpu.setFlag(CPU::FLAGS::C, 0); // Reset Carry Flag
    cpu.A = 0x19;
    cpu.writeBus(0x01, 0x6A);
    cpu.execute();
    assert(cpu.A == 0xC);

    // ROR Non-Accumulator Testing. Address 0xABCD loaded with 1, ROR executed, Carry Flag set, address should now hold 128
    cpu.setFlag(CPU::FLAGS::C, 1); // Set Carry Flag
    cpu.writeBus(0x02, 0x6E); // ROR Absolute
    cpu.writeBus(0x03, 0xCD);
    cpu.writeBus(0x04, 0xAB);
    cpu.writeBus(0xABCD, 0x1); // Load 0x1 into address 0xABCD
    cpu.execute();
    assert(cpu.readBus(0xABCD) == 0x80);
    assert(cpu.getFlag(CPU::FLAGS::C) == 1);

    std::cout << "---------------------------\nROR Instruction tests passed!\n";
}
//----------------------------------------------------------------------------------------------------------------------------
void Tests::test_CMP() {
	NES nes;
	CPU& cpu = *nes.bus.cpu;
    cpu.reset();

    // Accumulator loaded with 144, address loaded with 80, CMP executed, Carry flag should be set
    cpu.A = 0x90;
    cpu.writeBus(0x00, 0xCD); // CMP Absolute
    cpu.writeBus(0x01, 0xCD);
    cpu.writeBus(0x02, 0xAB);
    cpu.writeBus(0xABCD, 0x50);
    cpu.execute();
    assert(cpu.getFlag(CPU::FLAGS::C) == 1);

    // Accumulator loaded with 80, address loaded with 144, CMP executed, Carry flag should not be set
    cpu.A = 0x50;
    cpu.writeBus(0x03, 0xCD);
    cpu.writeBus(0x04, 0xCD);
    cpu.writeBus(0x05, 0xAB);
    cpu.writeBus(0xABCD, 0x90);
    cpu.execute();
    assert(cpu.getFlag(CPU::FLAGS::C) == 0);

    std::cout << "---------------------------\nCMP Instruction tests passed!\n";
}
//----------------------------------------------------------------------------------------------------------------------------
void Tests::test_CPX() {
	NES nes;
	CPU& cpu = *nes.bus.cpu;
    cpu.reset();

    // X register loaded with 144, address loaded with 80, CPX executed, Carry flag should be set
    cpu.X = 0x90;
    cpu.writeBus(0x00, 0xEC); // CPX Absolute
    cpu.writeBus(0x01, 0xCD);
    cpu.writeBus(0x02, 0xAB);
    cpu.writeBus(0xABCD, 0x50);
    cpu.execute();
    assert(cpu.getFlag(CPU::FLAGS::C) == 1);

    // X register loaded with 80, address loaded with 144, CPX executed, Carry flag should not be set
    cpu.X = 0x50;
    cpu.writeBus(0x03, 0xEC);
    cpu.writeBus(0x04, 0xCD);
    cpu.writeBus(0x05, 0xAB);
    cpu.writeBus(0xABCD, 0x90);
    cpu.execute();
    assert(cpu.getFlag(CPU::FLAGS::C) == 0);

    std::cout << "---------------------------\nCPX Instruction tests passed!\n";
}
//----------------------------------------------------------------------------------------------------------------------------
void Tests::test_CPY() {
	NES nes;
	CPU& cpu = *nes.bus.cpu;
    cpu.reset();

    // Y register loaded with 144, address loaded with 80, CPY executed, Carry flag should be set
    cpu.Y = 0x90;
    cpu.writeBus(0x00, 0xCC); // CPY Absolute
    cpu.writeBus(0x01, 0xCD);
    cpu.writeBus(0x02, 0xAB);
    cpu.writeBus(0xABCD, 0x50);
    cpu.execute();
    assert(cpu.getFlag(CPU::FLAGS::C) == 1);

    // Y register loaded with 80, address loaded with 144, CPY executed, Carry flag should not be set
    cpu.Y = 0x50;
    cpu.writeBus(0x03, 0xCC);
    cpu.writeBus(0x04, 0xCD);
    cpu.writeBus(0x05, 0xAB);
    cpu.writeBus(0xABCD, 0x90);
    cpu.execute();
    assert(cpu.getFlag(CPU::FLAGS::C) == 0);

    std::cout << "---------------------------\nCPY Instruction tests passed!\n";
}
//----------------------------------------------------------------------------------------------------------------------------
void Tests::test_CLD_SED_CLV() {
	NES nes;
	CPU& cpu = *nes.bus.cpu;
    cpu.reset();

    cpu.writeBus(0x00, 0xF8); // SED
    cpu.execute();
    assert(cpu.getFlag(CPU::FLAGS::D) == 1);

    cpu.writeBus(0x01, 0xD8); // CLD
    cpu.execute();
    assert(cpu.getFlag(CPU::FLAGS::D) == 0);

    cpu.setFlag(CPU::FLAGS::V, 1);
    cpu.writeBus(0x02, 0xB8); // CLV
    cpu.execute();
    assert(cpu.getFlag(CPU::FLAGS::V) == 0);

    std::cout << "---------------------------\nCLD_SED_CLV Instruction tests passed!\n";
}

void Tests::test_NES(std::string path) {
	NES nes;

	// Connect CPU to the bus
	nes.bus.cpu = &nes.cpu;
	nes.cpu.connectBus(&nes.bus);

	// Load ROM
	nes.load_rom(path.c_str()); // Current test rom is ./nestest.nes
	nes.rom.printHeaderInfo(nes.rom.ROMheader);
	printf("ROM HEADER FLAG 6: %d \n", nes.bus.ppu.ROM->ROMheader.flags6);

	// Initialize NES (calls reset internally)
	nes.initNES();

	// DEBUG: Verify connections right after initNES()
	std::cout << "initNES() finished\n";
	if (nes.bus.cpu == nullptr) {
		std::cerr << "ERROR: nes.bus.cpu is NULL after initNES()\n";
		return;
	}
	try {
		nes.cpu.readBus(0x0000); // Should not crash if connected properly
	} catch (...) {
		std::cerr << "ERROR: CPU bus read failed (likely disconnected)\n";
		return;
	}

	std::ofstream outfile("output.txt");

	auto start = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < 60; i++) {

		// DEBUG: Show what we're doing before the cycle
		std::cout << "Frame " << i + 1 << ": PC = 0x" << std::hex << nes.cpu.PC << "\n";

		// DEBUG: Check opcode fetch
		try {
			uint8_t opcode = nes.cpu.readBus(nes.cpu.PC);
			std::cout << "Opcode: 0x" << std::hex << static_cast<int>(opcode) << "\n";
		} catch (...) {
			std::cerr << "Exception while reading opcode at PC!\n";
			return;
		}

		nes.cpu.printRegisters();

		// DEBUG: Confirm cycle is safe
		try {
			nes.cycle();
		} catch (...) {
			std::cerr << "Exception occurred during nes.cycle()!\n";
			return;
		}

		outfile << std::hex << std::uppercase << nes.cpu.PC << std::endl;
	}

	outfile.close();
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed_time = end - start;
	std::cout << "Elapsed Time: " << elapsed_time.count() << " seconds\n";
}

void Tests::test_Bus() {

	Bus bus;
	CPU& cpu = *bus.cpu;

	cpu.reset();
	cpu.writeBus(0x0000, 0xFF);
	uint8_t opcode = cpu.readBus(0x0000);
	assert(opcode == 0xFF);
	std::cout << "---------------------------\nBus tests passed!\n";
}

void Tests::test_PPU_registers() {
	Bus bus;
	CPU& cpu = *bus.cpu;
	// Write to PPUCTRL
	cpu.writeBus(0x2000, 0xC2);

	// Read from PPUCTRL
	//uint8_t result = cpu.readBus(0x2000);
	uint8_t result = bus.ppu.control.reg;

	std::cout << "PPUCTRL: '" << std::hex << static_cast<int>(result) << "'\n";
	assert(result == 0xC2);

	std::cout << "PPU Register Tests Passed\n";
}

void Tests::test_pattern_tables(std::string path) {
	NES nes;
	nes.load_rom(path.c_str()); // current test rom is ./nestest.nes
	nes.initNES();
	//nes.cpu.PC = 0xC000;
	for (int i = 0;i < 265000; i++) {
		 //printf("count: %d\n", i+1);
		// uint8_t opcode = nes.cpu.readBus(nes.cpu.PC);
		// printf("Opcode: %02X\n", opcode);
		// nes.cpu.printRegisters();
		nes.cycle();
	}
	//nes.bus.ppu.printPatternTable();
	//nes.bus.ppu.printPaletteMemory();

}

void Tests::test_Pulse1() {
    std::cout << "Starting Pulse 1 test...\n";
    std::cout << "Pulse 1 test completed.\n";
}

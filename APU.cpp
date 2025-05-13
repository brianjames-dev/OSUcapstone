#include "APU.h"
#include <cstdio>  // For printf debugging

APU::APU() {
	reset();
}

void APU::reset() {
	registers.fill(0);  // Clear all registers
	frame_counter = 0;
//	printf("APU reset.\n");
}

void APU::write_register(uint16_t address, uint8_t value) {
	if (address >= 0x4000 && address <= 0x4017) {
		registers[address - 0x4000] = value; // Store value in register

		if (address == 0x4000 || address == 0x4004) {
			PulseChannel& pulse = (address == 0x4000) ? pulse1 : pulse2;
			pulse.duty = (value >> 6) & 0x03;  // Bits 6-7 set duty cycle
			pulse.constant_volume = value & 0x10;
			pulse.volume = value & 0x0F;
		} else if (address == 0x4002 || address == 0x4006) {
			PulseChannel& pulse = (address == 0x4002) ? pulse1 : pulse2;
			pulse.timer = (pulse.timer & 0xFF00) | value;  // Lower 8 bits
		} else if (address == 0x4003 || address == 0x4007) {
			PulseChannel& pulse = (address == 0x4003) ? pulse1 : pulse2;
			pulse.timer = (pulse.timer & 0x00FF) | ((value & 0x07) << 8);  // Upper 3 bits
			pulse.length_counter = (value >> 3) & 0x1F;  // Length counter
		} else if (address == 0x4015) {
			pulse1.enabled = value & 0x01;
			pulse2.enabled = value & 0x02;
		} else if (address == 0x4017) {
			frame_counter_mode = (value & 0x80) ? 5 : 4;
		}
	}
}

uint8_t APU::read_register(uint16_t address) {
	if (address == 0x4015) {
          uint8_t status = 0;
          if (pulse1.enabled) status |= 0x01;
          if (pulse2.enabled) status |= 0x02;
          return status;
	}

	//printf("Invalid APU register read: %04X\n", address);
	return 0;
}

void APU::clock() {
	// Increment frame counter
	frame_counter++;

	// 4-Step Frame Sequence
	if (frame_counter_mode == 4) {
    	// TODO: Implement 4-step sequence
	}

    // 5-Step Frame Sequence
    else if (frame_counter_mode == 5) {
		// TODO: Implement 5-step sequence
	}
}

void APU::step_envelope() {
	// TODO: Implement step_envelope
}
void APU::step_sweep() {
	// TODO: Implement step_sweep
}
void APU::step_length_counter() {
	// TODO: Implement step_length_counter
}
void PulseChannel::update_timer() {
	// TODO: Implement update_timer
}
void PulseChannel::update_envelope() {
	// TODO: Implement update_envelope
}
void PulseChannel::update_length_counter() {
	// TODO: Implement update_length_counter
}

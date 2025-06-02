#include "APU.h"
#include "Bus.h"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <cassert>
#include <cmath>

// Duty cycle waveforms
const uint8_t APU::DUTY_WAVEFORMS[4][8] = {
    {0, 1, 0, 0, 0, 0, 0, 0}, // 12.5%
    {0, 1, 1, 0, 0, 0, 0, 0}, // 25%
    {0, 1, 1, 1, 1, 0, 0, 0}, // 50%
    {1, 0, 0, 1, 1, 1, 1, 1}  // 75%
};


// Length counter lookup table (in frames, halved for 240 Hz clocking)
const uint8_t APU::LENGTH_TABLE[32] = {
    10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};


// Triangle Table
const int8_t TRIANGLE_WAVE[32] = {
    0,  1,  2,  3,  4,  5,  6,  7,
    8,  9, 10, 11, 12, 13, 14, 15,
   15, 14, 13, 12, 11, 10,  9,  8,
    7,  6,  5,  4,  3,  2,  1,  0
};


// NTSC noise timer periods (indexed by bits 0–3 of $400E)
const uint16_t NOISE_PERIOD_TABLE[16] = {
    4, 8, 16, 32, 64, 96, 128, 160,
    202, 254, 380, 508, 762, 1016, 2034, 4068
};


APU::APU() {
    pulse1_duty = 0;
    pulse1_sweep = 0;
    pulse1_timer_low = 0;
    pulse1_length = 0;
    pulse1_timer = 0;
    pulse1_timer_counter = 0.0f;
    pulse1_duty_pos = 0;
    pulse1_volume = 0;
    pulse1_enabled = false;

    envelope_loop = false;
    envelope_constant = false;
    envelope_period = 0;
    envelope_counter = 0;
    envelope_volume = 0;
    envelope_start = false;

    length_counter = 0;
    length_counter_halt = false;

    SDL_Init(SDL_INIT_AUDIO);
    SDL_AudioSpec want;
    SDL_zero(want);
    want.freq = 44100;
    want.format = AUDIO_F32SYS;
    want.channels = 1;
    want.samples = 1024;

    want.callback = [](void* userdata, Uint8* stream, int len) {
        APU* apu = static_cast<APU*>(userdata);
        float* fstream = reinterpret_cast<float*>(stream);
        int samples = len / sizeof(float);
        apu->generateSamples(fstream, samples);
    };
    want.userdata = this;

    audioDevice = SDL_OpenAudioDevice(nullptr, 0, &want, &audioSpec, 0);
    if (audioDevice == 0) {
        printf("Failed to open audio: %s\n", SDL_GetError());
        return;
    }
    SDL_PauseAudioDevice(audioDevice, 0);
}

APU::~APU() {
    SDL_CloseAudioDevice(audioDevice);
    SDL_Quit();
}


void APU::writeRegister(uint16_t address, uint8_t value) {
    // if (address >= 0x4000 && address <= 0x4007) {
    //     std::cout << "[APU] Write $" << std::hex << address << " = " << std::dec << (int)value << "\n";
    // }

    switch (address) {
		case 0x4000: // Duty, envelope control, and volume
            pulse1_duty = value;
            envelope_loop = (value & 0x20) != 0;
            envelope_constant = (value & 0x10) != 0;
            envelope_period = value & 0x0F;
            length_counter_halt = envelope_loop;
            if (envelope_constant) {
                pulse1_volume = envelope_period;
            } else {
                pulse1_volume = 15; // Default to max volume if not constant
            }
            break;

        case 0x4001: // Sweep
            pulse1_sweep = value;
            sweep_enabled1 = (value & 0x80) != 0;
            sweep_period1 = (value >> 4) & 0x07;
            sweep_negate1 = (value & 0x08) != 0;
            sweep_shift1 = value & 0x07;
            sweep_reload1 = true;
            break;

        case 0x4002: // Timer low
            pulse1_timer_low = value;
            pulse1_timer = (pulse1_timer & 0x0700) | value;
            break;

        case 0x4003: // Length counter load and timer high
			pulse1_length = value;
            pulse1_timer = (pulse1_timer & 0x00FF) | ((value & 0x07) << 8); // Bits 0-2: Timer high
            if (pulse1_enabled) {
                length_counter = LENGTH_TABLE[(value >> 3) & 0x1F];         // Bits 3-7: Length index
            }
            pulse1_duty_pos = 0;        // Reset waveform phase
            pulse1_enabled = true;      // Enable channel
            envelope_start = true;      // Restart envelope
            break;

        case 0x4004: // Pulse 2 duty, envelope control, volume
            pulse2_duty = value;
            envelope2_loop = (value & 0x20) != 0;
            envelope2_constant = (value & 0x10) != 0;
            envelope2_period = value & 0x0F;
            length_counter2_halt = envelope2_loop;
            if (envelope2_constant) {
                pulse2_volume = envelope2_period;
            } else {
                pulse2_volume = 15; // Default to max volume if not constant
            }
            break;

        case 0x4005: // Pulse 2 sweep
            pulse2_sweep = value;
            sweep_enabled2 = (value & 0x80) != 0;
            sweep_period2 = (value >> 4) & 0x07;
            sweep_negate2 = (value & 0x08) != 0;
            sweep_shift2 = value & 0x07;
            sweep_reload2 = true;
            break;

        case 0x4006: // Pulse 2 timer low
            pulse2_timer_low = value;
            pulse2_timer = (pulse2_timer & 0x0700) | value;
            break;

        case 0x4007: // Pulse 2 length counter load and timer high
            pulse2_length = value;
            pulse2_timer = (pulse2_timer & 0x00FF) | ((value & 0x07) << 8); // Bits 0-2
            if (pulse2_enabled) {
                length_counter2 = LENGTH_TABLE[(value >> 3) & 0x1F];
                // std::cout << "[Pulse2] Length counter loaded: " << (int)length_counter2 << "\n";
            }
            pulse2_duty_pos = 0;
            pulse2_enabled = true;
            envelope2_start = true;
            break;

        case 0x4008: // Triangle linear counter + control
            triangle_linear_control = value;
            triangle_linear_reload_value = value & 0x7F;
            triangle_enabled = true; // Enable on write for now (can refine later)
            break;

        case 0x400A: // Triangle timer low
            triangle_timer_low = value;
            triangle_timer = (triangle_timer & 0x0700) | value;
            break;

        case 0x400B: // Triangle length counter load and timer high
            triangle_length_load = value;
            triangle_timer = (triangle_timer & 0x00FF) | ((value & 0x07) << 8);
            triangle_length_counter = LENGTH_TABLE[(value >> 3) & 0x1F];
            triangle_linear_reload = true;
            triangle_wave_pos = 0; // Reset waveform phase
            break;

        case 0x400C: // Volume/envelope
            noise_volume_register = value;
            noise_envelope_loop = (value & 0x20) != 0;
            noise_envelope_constant = (value & 0x10) != 0;
            noise_envelope_period = value & 0x0F;
            noise_length_halt = noise_envelope_loop;
            if (noise_envelope_constant) {
                noise_volume = noise_envelope_period;
            } else {
                noise_volume = 15; // default max
            }
            break;

        case 0x400E: // Mode and timer period index
            noise_mode_period = value;
            break;

        case 0x400F: // Length counter load
            noise_length_load = value;
            noise_length_counter = LENGTH_TABLE[(value >> 3) & 0x1F];
            noise_envelope_start = true;
            noise_enabled = true;
            break;

        // case 0x4010: // Control (IRQ, loop, frequency index)
        //     dmc_control = value;
        //     dmc_timer_period = NOISE_PERIOD_TABLE[value & 0x0F]; // Reuse noise periods for now
        //     break;
        //
        // case 0x4011: // DAC direct output value (7 bits)
        //     dmc_output_level = value & 0x7F;
        //     break;
        //
        // case 0x4012: // Sample address (start)
        //     dmc_sample_address = value;
        //     dmc_current_address = 0xC000 + (value * 64);
        //     break;
        //
        // case 0x4013: // Sample length
        //     dmc_sample_length = value;
        //     dmc_bytes_remaining = (value * 16) + 1;
        //     break;
    }
}

uint8_t APU::readRegister(uint16_t address) {
    switch (address) {
    case 0x4015: {
            uint8_t status = 0x00;
            if (length_counter > 0)           status |= 0x01;
            if (length_counter2 > 0)          status |= 0x02;
            if (triangle_length_counter > 0)  status |= 0x04;
            if (noise_length_counter > 0)     status |= 0x08;
            // if (dmc_irq_flag)                 status |= 0x80;  // Bit 7 = DMC IRQ

            // dmc_irq_flag = false;  // Clear IRQ on read
            return status;
    }

    case 0x4017:
        return 0x00;  // You can implement the frame counter control/IRQ clear if needed

    default:
        return 0x00;
    }
}

void APU::generateSamples(float* stream, int length) {
    if ((pulse1_enabled == false || pulse1_timer == 0) &&
        (pulse2_enabled == false || pulse2_timer == 0) &&
        (triangle_enabled == false || triangle_timer == 0 || triangle_length_counter == 0 || triangle_linear_counter == 0) &&
        (noise_enabled == false || noise_timer == 0 || noise_length_counter == 0)) {

        for (int i = 0; i < length; i++) {
            stream[i] = 0.0f;
        }
        return;
    }

    float cpu_cycles_per_sample = 1789773.0f / audioSpec.freq;
    float timer_period1 = pulse1_timer + 1;
    float timer_period2 = pulse2_timer + 1;

    // Update noise timer period from mode value
    uint8_t noise_period_index = noise_mode_period & 0x0F;
    noise_timer = NOISE_PERIOD_TABLE[noise_period_index];

    if (noise_lfsr == 0) {
        // std::cout << "[Noise] LFSR got stuck! Resetting to 1\n";
        noise_lfsr = 1;
    }

    for (int i = 0; i < length; i++) {
        // --- Pulse 1 ---
        float sample1 = 0.0f;
        if (pulse1_enabled && timer_period1 > 0) {
            pulse1_timer_counter -= cpu_cycles_per_sample;
            if (pulse1_timer_counter <= 0) {
                pulse1_duty_pos = (pulse1_duty_pos + 1) % 8;
                pulse1_timer_counter += timer_period1;
            }

            uint8_t duty1 = (pulse1_duty >> 6) & 0x03;
            sample1 = DUTY_WAVEFORMS[duty1][pulse1_duty_pos] ? (pulse1_volume / 15.0f) : 0.0f;
        }

        // --- Pulse 2 ---
        float sample2 = 0.0f;
        if (pulse2_enabled && timer_period2 > 0) {
            pulse2_timer_counter -= cpu_cycles_per_sample;
            if (pulse2_timer_counter <= 0) {
                pulse2_duty_pos = (pulse2_duty_pos + 1) % 8;
                pulse2_timer_counter += timer_period2;
            }

            uint8_t duty2 = (pulse2_duty >> 6) & 0x03;
            sample2 = DUTY_WAVEFORMS[duty2][pulse2_duty_pos] ? (pulse2_volume / 15.0f) : 0.0f;
        }

        // --- Triangle ---
        float triangle_sample = 0.0f;
        if (triangle_enabled && triangle_timer > 0 &&
            triangle_length_counter > 0 && triangle_linear_counter > 0) {

            triangle_timer_counter -= cpu_cycles_per_sample;
            if (triangle_timer_counter <= 0) {
                triangle_wave_pos = (triangle_wave_pos + 1) % 32;
                triangle_timer_counter += (triangle_timer + 1);
            }

            triangle_sample = TRIANGLE_WAVE[triangle_wave_pos] / 15.0f;
        }

        // --- Noise ---
        float noise_sample = 0.0f;
        if (noise_enabled && noise_timer > 0 && noise_length_counter > 0) {
            noise_timer_counter -= cpu_cycles_per_sample;
            if (noise_timer_counter <= 0.0f) {
                bool mode = (noise_mode_period & 0x80) != 0;
                uint8_t bit0 = noise_lfsr & 0x1;
                uint8_t tap = mode ? ((noise_lfsr >> 6) & 0x1) : ((noise_lfsr >> 1) & 0x1);
                uint8_t feedback = bit0 ^ tap;

                noise_lfsr >>= 1;
                noise_lfsr |= (feedback << 14);
                noise_timer_counter += noise_timer;
            }

            noise_sample = (~noise_lfsr & 0x1) ? (noise_volume / 15.0f) : 0.0f;
        }

        // // --- DMC ---
        // float dmc_sample = 0.0f;
        //
        // if (dmc_enabled && dmc_timer_period > 0) {
        //     if (dmc_bits_remaining > 8) {
        //         std::cout << "[DMC] bits_remaining overflow! = " << (int)dmc_bits_remaining << " → clamping to 8\n";
        //         dmc_bits_remaining = 8;
        //     }
        //
        //     dmc_timer_counter -= cpu_cycles_per_sample;
        //     if (dmc_timer_counter <= 0.0f) {
        //         dmc_timer_counter += dmc_timer_period;
        //
        //         if (dmc_bits_remaining == 0) {
        //             if (dmc_bytes_remaining > 0) {
        //                 if (bus) {
        //                     dmc_sample_buffer = bus->read(dmc_current_address);
        //                     std::cout << "[DMC] Sample byte read: $" << std::hex << (int)dmc_sample_buffer << std::dec << "\n";
        //                 } else {
        //                     dmc_sample_buffer = 0x00;
        //                 }
        //
        //                 dmc_shift_register = dmc_sample_buffer;
        //                 dmc_bits_remaining = 8;
        //
        //                 // Clamp just in case
        //                 if (dmc_bits_remaining > 8) {
        //                     dmc_bits_remaining = 8;
        //                 }
        //
        //                 dmc_current_address = (dmc_current_address + 1) & 0xFFFF;
        //                 if (dmc_current_address < 0x8000) {
        //                     dmc_current_address = 0x8000;
        //                 }
        //
        //                 dmc_bytes_remaining--;
        //                 if (dmc_bytes_remaining == 0 && (dmc_control & 0x40)) {
        //                     dmc_bytes_remaining = (dmc_sample_length * 16) + 1;
        //                     dmc_current_address = 0xC000 + (dmc_sample_address * 64);
        //                 } else if (dmc_bytes_remaining == 0) {
        //                     dmc_enabled = false;
        //                 }
        //             }
        //         }
        //
        //         // Process one bit from the shift register (using clamped range)
        //         uint8_t bit = dmc_shift_register & 0x01;
        //         uint8_t max_level = 15;
        //
        //         if (bit && dmc_output_level < max_level) {
        //             dmc_output_level++;
        //         } else if (!bit && dmc_output_level > 0) {
        //             dmc_output_level--;
        //         }
        //
        //         dmc_shift_register >>= 1;
        //         dmc_bits_remaining--;
        //     }
        //
        //     dmc_sample = dmc_output_level / 15.0f;
        // }

        // --- Mix ---
        float pulse_out = 0.0f;
        if (sample1 != 0.0f || sample2 != 0.0f) {
            float pulse_mix = 95.88f / ((8128.0f / (sample1 * 15.0f + sample2 * 15.0f)) + 100);
            pulse_out = pulse_mix;
        }

        float tnd_mix = 0.0f;
        float tnd_input = (triangle_sample * 15.0f) + (noise_sample * 15.0f);  // + (dmc_sample * 15.0f);
        if (tnd_input != 0.0f) {
            tnd_mix = 159.79f / ((1.0f / tnd_input) + 100);
        }

        stream[i] = (pulse_out + tnd_mix) * 0.5f;
    }
}



void APU::clock() {
    frame_sequencer_counter++;

    if (frame_sequencer_counter == 7457) {
        clockEnvelopeAndLength();
    } else if (frame_sequencer_counter == 14913) {
        clockEnvelopeAndLength();
        clockSweepUnits();
    } else if (frame_sequencer_counter == 22371) {
        clockEnvelopeAndLength();
    } else if (frame_sequencer_counter == 29828) {
        clockEnvelopeAndLength();
        clockSweepUnits();
        frame_sequencer_counter = 0;
    }
}


void APU::clockEnvelopeAndLength() {
    // --- Pulse 1 Envelope ---
    if (envelope_start) {
        // std::cout << "[Envelope] Restarting P1 envelope\n";
        envelope_start = false;
        envelope_volume = 15;
        envelope_counter = envelope_period;
    } else if (!envelope_constant && envelope_counter > 0) {
        envelope_counter--;
        if (envelope_counter == 0) {
            if (envelope_volume > 0) {
                envelope_volume--;
            } else if (envelope_loop) {
                envelope_volume = 15;
            }
            envelope_counter = envelope_period;
        }
    }
    if (!envelope_constant) {
        pulse1_volume = envelope_volume;
    }

    // --- Pulse 1 Length Counter ---
    if (!length_counter_halt && length_counter > 0) {
        length_counter--;
        if (length_counter == 0) {
            pulse1_enabled = false;
        }
    }

    // --- Pulse 2 Envelope ---
    if (envelope2_start) {
        // std::cout << "[Envelope] Restarting P2 envelope\n";
        envelope2_start = false;
        envelope2_volume = 15;
        envelope2_counter = envelope2_period;
    } else if (!envelope2_constant && envelope2_counter > 0) {
        envelope2_counter--;
        if (envelope2_counter == 0) {
            if (envelope2_volume > 0) {
                envelope2_volume--;
            } else if (envelope2_loop) {
                envelope2_volume = 15;
            }
            envelope2_counter = envelope2_period;
        }
    }
    if (!envelope2_constant) {
        pulse2_volume = envelope2_volume;
    }

    // --- Pulse 2 Length Counter ---
    if (!length_counter2_halt && length_counter2 > 0) {
        // std::cout << "[Len2] Before: " << (int)length_counter2;
        length_counter2--;
        // std::cout << " After: " << (int)length_counter2 << "\n";

        if (length_counter2 == 0) {
            pulse2_enabled = false;
        }
    }

    // --- Triangle Linear Counter ---
    if (triangle_linear_reload) {
        triangle_linear_counter = triangle_linear_reload_value;
    } else if (triangle_linear_counter > 0) {
        triangle_linear_counter--;
    }

    if ((triangle_linear_control & 0x80) == 0) {
        triangle_linear_reload = false;
    }

    // --- Triangle Length Counter ---
    if ((triangle_linear_control & 0x80) == 0 && triangle_length_counter > 0) {
        triangle_length_counter--;
        if (triangle_length_counter == 0) {
            triangle_enabled = false;
        }
    }

    // --- Noise Envelope ---
    if (noise_envelope_start) {
        // std::cout << "[Envelope] Restarting Noise envelope\n";
        noise_envelope_start = false;
        noise_envelope_volume = 15;
        noise_envelope_counter = noise_envelope_period;
    } else if (!noise_envelope_constant && noise_envelope_counter > 0) {
        noise_envelope_counter--;
        if (noise_envelope_counter == 0) {
            if (noise_envelope_volume > 0) {
                noise_envelope_volume--;
            } else if (noise_envelope_loop) {
                noise_envelope_volume = 15;
            }
            noise_envelope_counter = noise_envelope_period;
        }
    }
    if (!noise_envelope_constant) {
        noise_volume = noise_envelope_volume;
    }

    // --- Noise Length Counter ---
    if (!noise_length_halt && noise_length_counter > 0) {
        noise_length_counter--;
        if (noise_length_counter == 0) {
            noise_enabled = false;
        }
    }
}


void APU::clockSweepUnits() {
    // --- Pulse 1 Sweep ---
    if (sweep_reload1) {
        sweep_counter1 = sweep_period1;
        sweep_reload1 = false;
    } else if (sweep_counter1 > 0) {
        sweep_counter1--;
    } else {
        sweep_counter1 = sweep_period1;
        if (sweep_enabled1 && sweep_shift1 > 0 && pulse1_timer >= 8) {
            uint16_t change = pulse1_timer >> sweep_shift1;
            uint16_t target;
            if (sweep_negate1) {
                target = pulse1_timer - change - 1;  // Subtract change AND 1
            } else {
                target = pulse1_timer + change;
            }            if (target <= 0x7FF) {
                pulse1_timer = target;
            } else {
                pulse1_enabled = false;
            }
        }
    }

    // --- Pulse 2 Sweep ---
    if (sweep_reload2) {
        sweep_counter2 = sweep_period2;
        sweep_reload2 = false;
    } else if (sweep_counter2 > 0) {
        sweep_counter2--;
    } else {
        sweep_counter2 = sweep_period2;
        if (sweep_enabled2 && sweep_shift2 > 0 && pulse2_timer >= 8) {
            uint16_t change = pulse2_timer >> sweep_shift2;
            uint16_t target = pulse2_timer + change; // always adds
            if (target <= 0x7FF) {
                pulse2_timer = target;
            } else {
                pulse2_enabled = false;
            }
        }
    }
}


void APU::reset() {
    // Reset Pulse 1 state
    pulse1_duty = 0;
    pulse1_sweep = 0;
    pulse1_timer_low = 0;
    pulse1_length = 0;
    pulse1_timer = 0;
    pulse1_timer_counter = 0.0f;
    pulse1_duty_pos = 0;
    pulse1_volume = 0;
    pulse1_enabled = false;

    envelope_loop = false;
    envelope_constant = false;
    envelope_period = 0;
    envelope_counter = 0;
    envelope_volume = 0;
    envelope_start = false;

    length_counter = 0;
    length_counter_halt = false;

    sweep_enabled1 = false;
    sweep_period1 = 0;
    sweep_shift1 = 0;
    sweep_negate1 = false;
    sweep_counter1 = 0;
    sweep_reload1 = false;

    // Reset Pulse 2 state
    pulse2_duty = 0;
    pulse2_sweep = 0;
    pulse2_timer_low = 0;
    pulse2_length = 0;
    pulse2_timer = 0;
    pulse2_timer_counter = 0.0f;
    pulse2_duty_pos = 0;
    pulse2_volume = 0;
    pulse2_enabled = false;

    envelope2_loop = false;
    envelope2_constant = false;
    envelope2_period = 0;
    envelope2_counter = 0;
    envelope2_volume = 0;
    envelope2_start = false;

    length_counter2 = 0;
    length_counter2_halt = false;

    sweep_enabled2 = false;
    sweep_period2 = 0;
    sweep_shift2 = 0;
    sweep_negate2 = false;
    sweep_counter2 = 0;
    sweep_reload2 = false;

    // Reset Triangle state
    triangle_linear_control = 0;
    triangle_timer_low = 0;
    triangle_length_load = 0;

    triangle_timer = 0;
    triangle_timer_counter = 0.0f;
    triangle_wave_pos = 0;
    triangle_linear_counter = 0;
    triangle_linear_reload_value = 0;
    triangle_linear_reload = false;
    triangle_length_counter = 0;
    triangle_enabled = false;

    // Reset Noise state
    noise_volume_register = 0;
    noise_mode_period = 0;
    noise_length_load = 0;

    noise_timer = 0;
    noise_timer_counter = 0.0f;
    noise_lfsr = 1; // Must initialize to 1 (not 0)
    noise_volume = 0;
    noise_envelope_period = 0;
    noise_envelope_counter = 0;
    noise_envelope_volume = 0;
    noise_envelope_loop = false;
    noise_envelope_constant = false;
    noise_envelope_start = false;
    noise_length_counter = 0;
    noise_length_halt = false;
    noise_enabled = false;

    // // Reset DMC state
    // dmc_control = 0;
    // dmc_output_level = 0;
    // dmc_sample_address = 0;
    // dmc_sample_length = 0;
    //
    // dmc_current_address = 0;
    // dmc_bytes_remaining = 0;
    // dmc_shift_register = 0;
    // dmc_bits_remaining = 0;
    // dmc_sample_buffer = 0;
    // dmc_sample_buffer_empty = true;
    // dmc_timer_counter = 0.0f;
    // dmc_timer_period = 428;
    // dmc_enabled = false;
}


#include "APU.h"
#include <cstdio>
#include <cstring>
#include <iostream>
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

        case 0x4001: // Sweep (not implemented)
            pulse1_sweep = value;
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

            // Other registers ($4004-$4017) would go here for other channels
    }
}

uint8_t APU::readRegister(uint16_t address) {
    switch (address) {
    case 0x4015:
        // Bit 0: Pulse 1 channel length counter > 0
            return pulse1_enabled ? 0x01 : 0x00;

    case 0x4017:
        // Frame counter read (not really used in most games)
            return 0x00; // or whatever placeholder value you want

    default:
        return 0x00;
    }
}

void APU::generateSamples(float* stream, int length) {
    // std::cout << "[APU] Generating audio samples...\n";

    if (!pulse1_enabled || pulse1_timer == 0) {
        for (int i = 0; i < length; i++) {
            stream[i] = 0.0f;
        }
        return;
    }

    float cpu_cycles_per_sample = 1789773.0f / audioSpec.freq;
    float timer_period = pulse1_timer + 1;

    for (int i = 0; i < length; i++) {
        pulse1_timer_counter -= cpu_cycles_per_sample;
        if (pulse1_timer_counter <= 0) {
            pulse1_duty_pos = (pulse1_duty_pos + 1) % 8;
            pulse1_timer_counter += timer_period;
        }

        uint8_t duty_cycle = (pulse1_duty >> 6) & 0x03;
        float sample = DUTY_WAVEFORMS[duty_cycle][pulse1_duty_pos] ?
                       (pulse1_volume / 15.0f) : 0.0f;

        stream[i] = sample * 0.3f;
    }
}

void APU::clock() {
    // Envelope clock (runs at ~240 Hz)
    static uint32_t frame_counter = 0;
    frame_counter++;
    if (frame_counter >= (1789773 / 240)) { // ~7459 CPU cycles per frame tick
        frame_counter = 0;

        // Update envelope
        if (envelope_start) {
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

        // Update length counter (also 240 Hz)
        if (!length_counter_halt && length_counter > 0) {
            length_counter--;
            if (length_counter == 0) {
                pulse1_enabled = false; // Silence channel
            }
        }
    }
}

void APU::reset() {
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
}


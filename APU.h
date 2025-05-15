#ifndef APU_H
#define APU_H

#include <cstdint>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

class APU {
public:
    APU();
    ~APU();

    void writeRegister(uint16_t address, uint8_t value);
    uint8_t readRegister(uint16_t address);
    void generateSamples(float* stream, int length);

    void clock();       // Step APU internals (envelope, length counter)
    void reset();       // Reset APU state

private:
    // Pulse 1 registers
    uint8_t pulse1_duty;        // $4000: Duty and envelope/volume
    uint8_t pulse1_sweep;       // $4001: Sweep (not implemented)
    uint8_t pulse1_timer_low;   // $4002: Timer low byte
    uint8_t pulse1_length;      // $4003: Length counter and timer high

    // Pulse 1 internal state
    uint16_t pulse1_timer;      // 11-bit timer value
    float pulse1_timer_counter; // Timing accumulator
    uint8_t pulse1_duty_pos;    // Duty cycle position
    uint8_t pulse1_volume;      // Current volume (from envelope or constant)
    bool pulse1_enabled;        // Channel enabled flag

    // Envelope state
    bool envelope_loop;         // $4000 bit 5: Loop envelope / length counter halt
    bool envelope_constant;     // $4000 bit 4: Constant volume flag
    uint8_t envelope_period;    // $4000 bits 0-3: Envelope period or constant volume
    uint8_t envelope_counter;   // Countdown for envelope decay
    uint8_t envelope_volume;    // Current envelope volume (0-15)
    bool envelope_start;        // Set when $4003 is written to restart envelope

    // Length counter state
    uint8_t length_counter;     // Counts down to silence channel
    bool length_counter_halt;   // From $4000 bit 5 (same as envelope_loop)

    SDL_AudioSpec audioSpec;
    SDL_AudioDeviceID audioDevice;

    static const uint8_t DUTY_WAVEFORMS[4][8];
    static const uint8_t LENGTH_TABLE[32]; // Lookup table for length counter
};


// TODO: Implement other sound channels here --> Triangle, Noise, DMC

#endif

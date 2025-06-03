#ifndef APU_H
#define APU_H

#include <cstdint>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
class Bus;

class APU {
public:
    APU();
    ~APU();

    void connectBus(Bus* b) { this->bus = b; }

    void writeRegister(uint16_t address, uint8_t value);
    uint8_t readRegister(uint16_t address);
    void generateSamples(float* stream, int length);

    void clockEnvelopeAndLength();
    void clockSweepUnits();
    bool dmc_irq_flag = false;
    bool frame_irq_flag = false;

    void clock();       // Step APU internals (envelope, length counter)
    void reset();       // Reset APU state

private:
    Bus* bus = nullptr;

    int frame_step = 0;
    int frame_sequencer_counter = 0;

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

    // Pulse 1 Envelope state
    bool envelope_loop;         // $4000 bit 5: Loop envelope / length counter halt
    bool envelope_constant;     // $4000 bit 4: Constant volume flag
    uint8_t envelope_period;    // $4000 bits 0-3: Envelope period or constant volume
    uint8_t envelope_counter;   // Countdown for envelope decay
    uint8_t envelope_volume;    // Current envelope volume (0-15)
    bool envelope_start;        // Set when $4003 is written to restart envelope

    // Pulse 1 Length counter state
    uint8_t length_counter;     // Counts down to silence channel
    bool length_counter_halt;   // From $4000 bit 5 (same as envelope_loop)

    // Pulse 1 sweep unit state
    bool sweep_enabled1;
    uint8_t sweep_period1;
    uint8_t sweep_shift1;
    bool sweep_negate1;
    uint8_t sweep_counter1;
    bool sweep_reload1;

    // Pulse 2 registers
    uint8_t pulse2_duty;
    uint8_t pulse2_sweep;
    uint8_t pulse2_timer_low;
    uint8_t pulse2_length;

    // Pulse 2 internal state
    uint16_t pulse2_timer;
    float pulse2_timer_counter;
    uint8_t pulse2_duty_pos;
    uint8_t pulse2_volume;
    bool pulse2_enabled;

    // Pulse 2 Envelope state
    bool envelope2_loop;
    bool envelope2_constant;
    uint8_t envelope2_period;
    uint8_t envelope2_counter;
    uint8_t envelope2_volume;
    bool envelope2_start;

    // Pulse 2 Length counter state
    uint8_t length_counter2;
    bool length_counter2_halt;

    // Pulse 2 sweep unit state
    bool sweep_enabled2;
    uint8_t sweep_period2;
    uint8_t sweep_shift2;
    bool sweep_negate2;
    uint8_t sweep_counter2;
    bool sweep_reload2;

    // Triangle channel registers
    uint8_t triangle_linear_control; // $4008
    uint8_t triangle_timer_low;      // $400A
    uint8_t triangle_length_load;    // $400B

    // Triangle internal state
    uint16_t triangle_timer;         // 11-bit timer
    float triangle_timer_counter;    // Accumulator
    uint8_t triangle_wave_pos;       // Position in 32-step waveform
    uint8_t triangle_linear_counter;
    uint8_t triangle_linear_reload_value;
    bool triangle_linear_reload;
    uint8_t triangle_length_counter;
    bool triangle_enabled;

    // Noise channel registers
    uint8_t noise_volume_register;   // $400C
    uint8_t noise_mode_period;       // $400E
    uint8_t noise_length_load;       // $400F

    // Noise internal state
    uint16_t noise_timer;
    float noise_timer_counter;
    uint16_t noise_lfsr;             // 15-bit LFSR
    uint8_t noise_volume;
    uint8_t noise_envelope_period;
    uint8_t noise_envelope_counter;
    uint8_t noise_envelope_volume;
    bool noise_envelope_loop;
    bool noise_envelope_constant;
    bool noise_envelope_start;
    uint8_t noise_length_counter;
    bool noise_length_halt;
    bool noise_enabled;

    // DMC channel registers
    uint8_t dmc_control;      // $4010: IRQ, loop, rate
    uint8_t dmc_output_level; // $4011: DAC
    uint8_t dmc_sample_address; // $4012
    uint8_t dmc_sample_length;  // $4013

    // DMC internal state
    uint16_t dmc_current_address;
    uint16_t dmc_bytes_remaining;
    uint8_t dmc_shift_register;
    uint8_t dmc_bits_remaining;
    uint8_t dmc_sample_buffer;
    bool dmc_sample_buffer_empty;
    float dmc_timer_counter;
    uint16_t dmc_timer_period;
    bool dmc_enabled;

    SDL_AudioSpec audioSpec;
    SDL_AudioDeviceID audioDevice;

    static const uint8_t DUTY_WAVEFORMS[4][8];
    static const uint8_t LENGTH_TABLE[32]; // Lookup table for length counter
};


// TODO: Implement other sound channels here --> Triangle, Noise, DMC

#endif

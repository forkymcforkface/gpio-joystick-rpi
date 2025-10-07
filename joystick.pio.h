#pragma once

#include "hardware/pio.h"

// --- PIO Program for Player 1 (9 buttons) ---
static const uint16_t joystick_9bit_program_instructions[] = {
    // .program joystick_9bit_reader
    0x4009, //  0: in    pins, 9
    0x8000, //  1: push   noblock
    0x0001, //  2: jmp    1
};

static const struct pio_program joystick_9bit_program = {
    .instructions = joystick_9bit_program_instructions,
    .length = 3,
    .origin = -1,
};


// --- PIO Program for Player 2 (8 buttons) ---
static const uint16_t joystick_8bit_program_instructions[] = {
    // .program joystick_8bit_reader
    0x4008, //  0: in    pins, 8
    0x8000, //  1: push   noblock
    0x0001, //  2: jmp    1
};

static const struct pio_program joystick_8bit_program = {
    .instructions = joystick_8bit_program_instructions,
    .length = 3,
    .origin = -1,
};

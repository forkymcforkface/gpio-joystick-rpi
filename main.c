#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>
#include "piolib.h"
#include "joystick.pio.h"

// --- Configuration ---
#define P1_FIRST_GPIO_PIN 0  // Player 1 starts at GPIO 0
#define P1_NUM_BUTTONS 9     // 8 standard + 1 Home button

#define P2_FIRST_GPIO_PIN 9  // Player 2 starts at GPIO 9
#define P2_NUM_BUTTONS 8     // 8 standard buttons

// Player 1 key map, including the Home button
const int key_map_p1[P1_NUM_BUTTONS] = {
    BTN_SOUTH, BTN_EAST, BTN_NORTH, BTN_WEST,
    BTN_START, BTN_SELECT, BTN_TL, BTN_TR,
    BTN_MODE // Home button
};

// Player 2 key map
const int key_map_p2[P2_NUM_BUTTONS] = {
    BTN_SOUTH, BTN_EAST, BTN_NORTH, BTN_WEST,
    BTN_START, BTN_SELECT, BTN_TL, BTN_TR
};

// Helper to set up a virtual joystick device
struct libevdev_uinput* setup_joystick_device(const char* name, const int key_map[], int num_buttons) {
    struct libevdev *dev = libevdev_new();
    libevdev_set_name(dev, name);
    libevdev_enable_event_type(dev, EV_KEY);
    for (int i = 0; i < num_buttons; i++) {
        libevdev_enable_event_code(dev, EV_KEY, key_map[i], NULL);
    }

    struct libevdev_uinput *uinput_dev;
    int err = libevdev_uinput_create_from_device(dev, LIBEVDEV_UINPUT_OPEN_MANAGED, &uinput_dev);
    libevdev_free(dev); // Context is no longer needed
    if (err != 0) {
        fprintf(stderr, "Failed to create uinput device for %s (error %d).\n", name, err);
        return NULL;
    }
    printf("Virtual joystick created: %s at %s\n", name, libevdev_uinput_get_devnode(uinput_dev));
    return uinput_dev;
}

// Helper to process input for a single player
void process_player_input(struct pio_prog *prog, struct libevdev_uinput *uinput_dev, uint32_t *last_state, const int key_map[], int num_buttons) {
    if (pio_sm_get_rx_fifo_level(prog->sm) > 0) {
        uint32_t current_state = pio_sm_get(prog->sm);
        uint32_t changed_bits = *last_state ^ current_state;

        if (changed_bits) {
            for (int i = 0; i < num_buttons; i++) {
                if ((changed_bits >> i) & 1) {
                    int value = ((current_state >> i) & 1) ? 0 : 1; // GPIO low is pressed
                    libevdev_uinput_write_event(uinput_dev, EV_KEY, key_map[i], value);
                }
            }
            libevdev_uinput_write_event(uinput_dev, EV_SYN, SYN_REPORT, 0);
            *last_state = current_state;
        }
    }
}

int main() {
    struct pio_prog prog1, prog2;
    struct libevdev_uinput *uinput_dev1, *uinput_dev2;
    uint32_t last_state1 = 0, last_state2 = 0;

    // 1. Initialize PIO system
    if (pio_init()) {
        fprintf(stderr, "Failed to initialize PIO\n");
        return 1;
    }

    // 2. Setup Player 1 (9-button program)
    if (pio_add_program(&prog1, &joystick_9bit_program)) {
        fprintf(stderr, "Failed to add 9-bit PIO program\n");
        return 1;
    }
    pio_sm_set_in_pins(prog1.sm, P1_FIRST_GPIO_PIN);
    pio_sm_init(prog1.sm, prog1.offset, NULL);
    pio_sm_set_enabled(prog1.sm, true);
    uinput_dev1 = setup_joystick_device("PIO Joystick P1", key_map_p1, P1_NUM_BUTTONS);
    if (!uinput_dev1) return 1;

    // 3. Setup Player 2 (8-button program)
    if (pio_add_program(&prog2, &joystick_8bit_program)) {
        fprintf(stderr, "Failed to add 8-bit PIO program\n");
        return 1;
    }
    pio_sm_set_in_pins(prog2.sm, P2_FIRST_GPIO_PIN);
    pio_sm_init(prog2.sm, prog2.offset, NULL);
    pio_sm_set_enabled(prog2.sm, true);
    uinput_dev2 = setup_joystick_device("PIO Joystick P2", key_map_p2, P2_NUM_BUTTONS);
    if (!uinput_dev2) return 1;

    // 4. Main Event Loop
    printf("Listening for joystick input from both players...\n");
    while (1) {
        process_player_input(&prog1, uinput_dev1, &last_state1, key_map_p1, P1_NUM_BUTTONS);
        process_player_input(&prog2, uinput_dev2, &last_state2, key_map_p2, P2_NUM_BUTTONS);
        usleep(1000); // 1ms sleep to prevent 100% CPU usage
    }

    // Cleanup (unreachable in this daemon, but good practice)
    libevdev_uinput_destroy(uinput_dev1);
    libevdev_uinput_destroy(uinput_dev2);
    pio_remove_program(&prog1);
    pio_remove_program(&prog2);
    pio_exit();
    return 0;
}

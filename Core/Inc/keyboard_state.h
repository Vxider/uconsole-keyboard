#ifndef KEYBOARD_STATE_H
#define KEYBOARD_STATE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    // Current layer
    uint8_t layer;

    // If FN key is pressed
    uint8_t fn;

    // Current modifier keys pressed
    uint16_t mod_keys_on;

    // Backlight level
    uint8_t backlight;

    // LEDs
    uint32_t leds_timer;
    uint16_t leds_interfal;

    // For double press features
    uint16_t last_pressed_key;
    uint32_t last_pressed_time;
    uint32_t last_activity_time;

    // Options
    uint8_t fn_lock;                    /* Invert FN key functionality for F1-F12 */
    uint8_t double_p_to_brace_left;     /* Double press P key to insert brace left */
} KEYBOARD_STATE;

extern KEYBOARD_STATE keyboard_state;

#endif


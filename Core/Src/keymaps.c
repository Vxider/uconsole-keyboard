#include "keymaps.h"
#include "keyboard_state.h"
#include "hid_keyboard.h"
#include "hid_mouse.h"
#include "hid_consumer.h"
#include "hid_gamepad.h"
#include "trackball.h"
#include "main.h"
#include "stm32f1xx_hal.h"

KEYBOARD_STATE keyboard_state;

static uint8_t  layer_enabled[LAYERS_NUM] = {0};
static uint16_t matrix_maps[LAYERS_NUM * 2][MATRIX_KEYS] = {0};
static uint16_t keys_maps[LAYERS_NUM * 2][NON_MATRIX_KEYS] = {0};

/*
// Mappings for the keyboard matrix
const uint16_t matrix_maps[][MATRIX_KEYS] = {
    [DEF_LAYER] = {
        KEY_KEYPAD_MINUS,      KEY_KEYPAD_PLUS,       CONSUMER_VOLUME_DOWN,     KEY_GRAVE,              KEY_LEFT_BRACE,    KEY_RIGHT_BRACE,   KEY_MINUS,        KEY_EQUAL,
        KEY_1,                 KEY_2,                 KEY_3,                    KEY_4,                  KEY_5,             KEY_6,             KEY_7,            KEY_8,
        KEY_9,                 KEY_0,                 KEY_ESC,                  KEY_TAB,                KEY_NONE,          KEY_NONE,          KEY_NONE,         KEY_NONE,
        KEY_Q,                 KEY_W,                 KEY_E,                    KEY_R,                  KEY_T,             KEY_Y,             KEY_U,            KEY_I,
        KEY_O,                 KEY_P,                 KEY_A,                    KEY_S,                  KEY_D,             KEY_F,             KEY_G,            KEY_H,
        KEY_J,                 KEY_K,                 KEY_L,                    KEY_Z,                  KEY_X,             KEY_C,             KEY_V,            KEY_B,
        KEY_N,                 KEY_M,                 KEY_COMMA,                KEY_DOT,                KEY_SLASH,         KEY_BACKSLASH,     KEY_SEMICOLON,    KEY_APOSTROPHE,
        KEY_BACKSPACE,         KEY_RETURN,            SK_FN_KEY,                SK_FN_KEY,              KEY_SPACE,         KEY_NONE,          KEY_NONE,         KEY_NONE
    },
    [FN_LAYER] = {
        KEY_PRNT_SCRN,         KEY_PAUSE,             CONSUMER_MUTE,            0,                      0,                 0,                 KEY_F11,          KEY_F12,
        KEY_F1,                KEY_F2,                KEY_F3,                   KEY_F4,                 KEY_F5,            KEY_F6,            KEY_F7,           KEY_F8,
        KEY_F9,                KEY_F10,               SK_KEYBOARD_LOCK,         KEY_CAPS_LOCK,          0,                 0,                 0,                0,
        0,                     0,                     0,                        0,                      0,                 0,                 KEY_PAGE_UP,      KEY_INSERT,
        0,                     0,                     0,                        0,                      0,                 0,                 0,                KEY_HOME,
        KEY_END,               KEY_PAGE_DOWN,         0,                        0,                      0,                 0,                 0,                0,
        0,                     0,                     CONSUMER_BRIGHTNESS_DOWN, CONSUMER_BRIGHTNESS_UP, 0,                 0,                 0,                0,
        KEY_DELETE,            0,                     0,                        0,                      SK_KEYBOARD_LIGHT, 0,                 0,                0
    },
    [GAME_LAYER] = {
        GAMEPAD_BUTTON_5,      GAMEPAD_BUTTON_6,      0,                        0,                      0,                 0,                 0,                0,
        0,                     0,                     0,                        0,                      0,                 0,                 0,                0,
        0,                     0,                     0,                        0,                      0,                 0,                 0,                0,
        0,                     0,                     0,                        0,                      0,                 0,                 0,                0,
        0,                     0,                     0,                        0,                      0,                 0,                 0,                0,
        0,                     0,                     0,                        0,                      0,                 0,                 0,                0,
        0,                     0,                     0,                        0,                      0,                 0,                 0,                0,
        0,                     0,                     0,                        0,                      0,                 0,                 0,                0
    }
};

// Mappings for the non-matrix keys
const uint16_t keys_maps[][NON_MATRIX_KEYS] = {
    [DEF_LAYER] = {
        MOUSE_MIDDLE,           // Trackball button
        KEY_UP_ARROW,           // Up
        KEY_DOWN_ARROW,         // Down
        KEY_LEFT_ARROW,         // Left
        KEY_RIGHT_ARROW,        // Right
        MOUSE_FORWARD,          // Gamepad A
        MOUSE_BACK,             // Gamepad B
        KEY_LEFT_GUI,           // Gamepad X
        MOUSE_LEFT,             // Gamepad Y
        KEY_LEFT_SHIFT,         // Left shift
        KEY_RIGHT_SHIFT,        // Right shift
        KEY_LEFT_CTRL,          // Left ctrl
        KEY_RIGHT_CTRL,         // Right ctrl
        KEY_LEFT_ALT,           // Left alt
        MOUSE_LEFT,             // Gamepad L
        KEY_RIGHT_ALT,          // Right alt
        MOUSE_RIGHT,            // Gamepad R
    },
    [FN_LAYER] = {
        MOUSE_MIDDLE,           // Trackball button
        KEY_PAGE_UP,            // Up
        KEY_PAGE_DOWN,          // Down
        KEY_HOME,               // Left
        KEY_END,                // Right
        KEY_KEYPAD_PLUS,        // Gamepad A
        KEY_KEYPAD_MINUS,       // Gamepad B
        KEY_KEYPAD_ASTERISK,    // Gamepad X
        KEY_KEYPAD_SLASH,       // Gamepad Y
        0,                      // Left shift
        0,                      // Right shift
        0,                      // Left ctrl
        0,                      // Right ctrl
        KEY_LEFT_GUI,           // Left alt
        0,                      // Gamepad L
        0,                      // Right alt
        0,                      // Gamepad R
    },
    [GAME_LAYER] = {
        0,                      // Trackball button
        GAMEPAD_UP,             // Up
        GAMEPAD_DOWN,           // Down
        GAMEPAD_LEFT,           // Left
        GAMEPAD_RIGHT,          // Right
        GAMEPAD_BUTTON_1,       // Gamepad A
        GAMEPAD_BUTTON_2,       // Gamepad B
        GAMEPAD_BUTTON_3,       // Gamepad X
        GAMEPAD_BUTTON_4,       // Gamepad Y
        0,                      // Left shift
        0,                      // Right shift
        0,                      // Left ctrl
        0,                      // Right ctrl
        0,                      // Left alt
        0,                      // Gamepad L
        0,                      // Right alt
        0,                      // Gamepad R
    }
};
*/

static uint16_t matrix_pick_map[MATRIX_KEYS] = {0};
static uint16_t non_matrix_pick_map[NON_MATRIX_KEYS] = {0};

static void do_the_key(uint16_t k, uint8_t mode)
{
    switch (k) {
        case SK_FN_KEY:
            keyboard_state.fn = mode == KEY_PRESSED;
            break;

        case CONSUMER_VOLUME_DOWN:
            /* Override volume down button, so shift+vol_down = vol_up */
            if (mode == KEY_PRESSED) {
                if (keyboard_state.mod_keys_on & (KEY_LEFT_SHIFT | KEY_RIGHT_SHIFT)) {
                    // Shift was pressed - increase volume, but release the shift first
                    hid_keyboard_modifier(KEY_LEFT_SHIFT | KEY_RIGHT_SHIFT, KEY_RELEASED);
                    hid_consumer_button(CONSUMER_VOLUME_UP, KEY_PRESSED);
                } else {
                    // No shift - decrease volume
                    hid_consumer_button(CONSUMER_VOLUME_DOWN, KEY_PRESSED);
                }
            } else {
                hid_consumer_button(CONSUMER_VOLUME_UP, KEY_RELEASED);
                hid_consumer_button(CONSUMER_VOLUME_DOWN, KEY_RELEASED);
            }
            break;

        case SK_KEYBOARD_LOCK:
            if (mode == KEY_PRESSED) {
                keyboard_state.fn_lock = !keyboard_state.fn_lock;
                leds_blink(keyboard_state.fn_lock + 1, LEDS_BLINK_PERIOD_LONG);
            }
            break;

        case SK_KEYBOARD_LIGHT:
            if (mode == KEY_PRESSED) {
                keyboard_state.backlight++;
                if (keyboard_state.backlight >= (sizeof(backlight_vals) / sizeof(backlight_vals[0]))) {
                    keyboard_state.backlight = 0;
                }
                // PWM will be set in main loop
            }
            break;

        default:
            if (k == KEY_NONE) {
                break;
            } else if (k & CONSUMER_KEY_FLAG) {
                hid_consumer_button(k, mode);
            } else if (k & MODIFIER_KEY_FLAG) {
                hid_keyboard_modifier(k, mode);
                if (mode == KEY_PRESSED) {
                    keyboard_state.mod_keys_on |= (uint8_t)k;
                } else {
                    keyboard_state.mod_keys_on &= ~(uint8_t)k;
                }                
            } else if (k & MOUSE_BUTTON_FLAG) {
                hid_mouse_button(k, mode);
            } else if (k & GAMEPAD_BUTTON_FLAG) {
                hid_gamepad_button(k, mode);
            } else if (k < 0x100) {
                hid_keyboard_button(k, mode);
            }
            break;
    }
}

void switch_layer(uint8_t layer)
{
    if (layer >= LAYERS_NUM || !layer_enabled[layer]) {
        return;
    }
    keyboard_state.layer = layer;
    trackball_load_layer_config();
    leds_blink(layer + 1, LEDS_BLINK_PERIOD_SHORT);
}

void matrix_action(uint8_t row, uint8_t col, uint8_t mode)
{
    uint16_t k;
    uint8_t addr = row * MATRIX_COLS + col;
    uint8_t fn = keyboard_state.fn ? 1 : 0;

    if ((keyboard_state.mod_keys_on & KEY_LEFT_CTRL) && (keyboard_state.mod_keys_on & KEY_RIGHT_CTRL) && mode == KEY_PRESSED) {
        switch (addr)
        {
            case BUTTON_1:
                switch_layer(0); return;
            case BUTTON_2:
                switch_layer(1); return;
            case BUTTON_3:
                switch_layer(2); return;
            case BUTTON_4:
                switch_layer(3); return;
            case BUTTON_5:
                switch_layer(4); return;
            case BUTTON_6:
                switch_layer(5); return;
            case BUTTON_7:
                switch_layer(6); return;
            case BUTTON_8:
                switch_layer(7); return;
            case BUTTON_9:
                switch_layer(8); return;
            case BUTTON_0:
                switch_layer(9); return;
            default:
                break;
        }
    }

    if (keyboard_state.fn_lock) {
        switch (addr)
        {
            case BUTTON_1:
            case BUTTON_2:
            case BUTTON_3:
            case BUTTON_4:
            case BUTTON_5:
            case BUTTON_6:
            case BUTTON_7:
            case BUTTON_8:
            case BUTTON_9:
            case BUTTON_0:
            case BUTTON_MINUS:
            case BUTTON_EQUAL:
                fn = !fn;
                break;
            default:
                break;
        }
    }

    k = matrix_maps[keyboard_state.layer * 2 + fn][addr];
    if (k == KEY_NONE) {
        return;
    }
    if (!k && keyboard_state.layer > 0) {
        // Non-default layer, mapping is empty, try default layer
        k = matrix_maps[0 * 2 + fn][addr];
        if (k == KEY_NONE) {
            return;
        }
    }
    if (!k && fn) {
        // Still no mapping? Try non-fn layer
        k = matrix_maps[0][addr];
    }
    if (!k || k == KEY_NONE) {
        return;
    }
    
    if (mode == KEY_PRESSED) {
        if (matrix_pick_map[addr] == 0) {
            matrix_pick_map[addr] = k;
        }
        do_the_key(k, KEY_PRESSED);
        keyboard_state.last_pressed_key = k;
        keyboard_state.last_pressed_time = HAL_GetTick();
    } else {
        if (matrix_pick_map[addr] == 0) {
            // No stored value, use provided HID code
            do_the_key(k, KEY_RELEASED);
        } else {
            // Use stored value (already HID code)
            uint16_t stored_key = matrix_pick_map[addr];
            matrix_pick_map[addr] = 0;
            do_the_key(stored_key, KEY_RELEASED);
        }
    }
}

void non_matrix_action(uint8_t col, uint8_t mode)
{
    uint16_t k;
    uint8_t fn = keyboard_state.fn ? 1 : 0;

    /* Emergency recovery mode */
    if ((keyboard_state.mod_keys_on & KEY_LEFT_CTRL) 
        && (keyboard_state.mod_keys_on & KEY_RIGHT_CTRL) 
        && mode == KEY_PRESSED 
        && col == (BUTTON_TRACKBALL - MATRIX_KEYS)) {
            jump_to_bootloader();
    }

    k = keys_maps[keyboard_state.layer * 2 + fn][col];
    if (k == KEY_NONE) {
        return;
    }
    if (!k && keyboard_state.layer > 0) {
        // Non-default layer, mapping is empty, try default layer
        k = keys_maps[0 * 2 + fn][col];
        if (k == KEY_NONE) {
            return;
        }
    }
    if (!k && fn) {
        // Still no mapping? Try non-fn layer
        k = keys_maps[0][col];
    }
    if (!k || k == KEY_NONE) {
        return;
    }
    
    if (mode == KEY_PRESSED) {
        if (non_matrix_pick_map[col] == 0) {
            non_matrix_pick_map[col] = k;
        }
        do_the_key(k, KEY_PRESSED);
        keyboard_state.last_pressed_key = k;
        keyboard_state.last_pressed_time = HAL_GetTick();
    } else {
        if (non_matrix_pick_map[col] == 0) {
            do_the_key(k, KEY_RELEASED);
        } else {
            do_the_key(non_matrix_pick_map[col], KEY_RELEASED);
            non_matrix_pick_map[col] = 0;
        }
    }
}

void bind_button(uint8_t layer, uint8_t fn, uint16_t button, uint16_t key) {
    layer_enabled[layer] = 1;
    if (button < MATRIX_KEYS)
        matrix_maps[(layer * 2) + (fn & 1)][button] = key;
    else if (button < MATRIX_KEYS + NON_MATRIX_KEYS)
        keys_maps[(layer * 2) + (fn & 1)][button - MATRIX_KEYS] = key;
}

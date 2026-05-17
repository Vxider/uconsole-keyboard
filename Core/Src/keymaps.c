#include "keymaps.h"
#include "keyboard_state.h"
#include "hid_keyboard.h"
#include "hid_mouse.h"
#include "hid_consumer.h"
#include "hid_gamepad.h"
#include "hid_vendor.h"
#include "trackball.h"
#include "main.h"
#include "stm32f1xx_hal.h"

KEYBOARD_STATE keyboard_state;

static uint8_t  layer_enabled[LAYERS_NUM] = {0};
static uint16_t matrix_maps[LAYERS_NUM * 2][MATRIX_KEYS] = {0};
static uint16_t keys_maps[LAYERS_NUM * 2][NON_MATRIX_KEYS] = {0};
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
                if (keyboard_state.mod_keys_on & (KEY_SHIFT_LEFT | KEY_SHIFT_RIGHT)) {
                    // Shift was pressed - increase volume, but release the shift first
                    hid_keyboard_modifier(KEY_SHIFT_LEFT | KEY_SHIFT_RIGHT, KEY_RELEASED);
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
                fn_lock_set(!keyboard_state.fn_lock);
                hid_vendor_schedule_send();
            }
            break;

        case SK_KEYBOARD_LIGHT:
            if (mode == KEY_PRESSED) {
                keyboard_state.backlight++;
                if (keyboard_state.backlight >= (sizeof(backlight_vals) / sizeof(backlight_vals[0]))) {
                    keyboard_state.backlight = 0;
                }
                hid_vendor_schedule_send();
                // PWM will be set in main loop
            }
            break;

#if REPLACE_DOUBLE_SEMICOLON_WITH_APOSTROPHE
        case KEY_SEMICOLON:
            if (mode == KEY_PRESSED) {
                // Double press feature
                // ;; -> ' (жж -> э)
                // :: -> " (ЖЖ -> Э)
                if (HAL_GetTick() - keyboard_state.last_pressed_time < DOUBLE_PRESS_TIME_MS && keyboard_state.last_pressed_key == KEY_SEMICOLON) {
                    // Release the shift keys first if they are pressed
                    if (keyboard_state.mod_keys_on & KEY_SHIFT_LEFT) {
                        hid_keyboard_modifier(KEY_SHIFT_LEFT, KEY_RELEASED);
                    }
                    if (keyboard_state.mod_keys_on & KEY_SHIFT_RIGHT) {
                        hid_keyboard_modifier(KEY_SHIFT_RIGHT, KEY_RELEASED);
                    }
                    hid_keyboard_button(KEY_BACKSPACE, KEY_PRESSED);
                    hid_keyboard_button(KEY_BACKSPACE, KEY_RELEASED);
                    // Press the shift keys again
                    if (keyboard_state.mod_keys_on & KEY_SHIFT_LEFT) {
                        hid_keyboard_modifier(KEY_SHIFT_LEFT, KEY_PRESSED);
                    }
                    if (keyboard_state.mod_keys_on & KEY_SHIFT_RIGHT) {
                        hid_keyboard_modifier(KEY_SHIFT_RIGHT, KEY_PRESSED);
                    }    
                    hid_keyboard_button(KEY_APOSTROPHE, KEY_PRESSED);
                    hid_keyboard_button(KEY_APOSTROPHE, KEY_RELEASED);
                    keyboard_state.last_pressed_key = KEY_NONE;
                } else {
                    hid_keyboard_button(k, mode);
                }
            } else {
                hid_keyboard_button(k, mode);
            }
            break;
#endif

        case KEY_P:
            if (mode == KEY_PRESSED) {
                // Double press feature
                // pp -> [ (з -> Х)
                // PP -> { (З -> Х)
                if (keyboard_state.double_p_to_brace_left && HAL_GetTick() - keyboard_state.last_pressed_time < DOUBLE_PRESS_TIME_MS && keyboard_state.last_pressed_key == KEY_P) {
                    // Release the shift keys first if they are pressed
                    if (keyboard_state.mod_keys_on & KEY_SHIFT_LEFT) {
                        hid_keyboard_modifier(KEY_SHIFT_LEFT, KEY_RELEASED);
                    }
                    if (keyboard_state.mod_keys_on & KEY_SHIFT_RIGHT) {
                        hid_keyboard_modifier(KEY_SHIFT_RIGHT, KEY_RELEASED);
                    }
                    hid_keyboard_button(KEY_BACKSPACE, KEY_PRESSED);
                    hid_keyboard_button(KEY_BACKSPACE, KEY_RELEASED);
                    // Press the shift keys again
                    if (keyboard_state.mod_keys_on & KEY_SHIFT_LEFT) {
                        hid_keyboard_modifier(KEY_SHIFT_LEFT, KEY_PRESSED);
                    }
                    if (keyboard_state.mod_keys_on & KEY_SHIFT_RIGHT) {
                        hid_keyboard_modifier(KEY_SHIFT_RIGHT, KEY_PRESSED);
                    }    
                    hid_keyboard_button(KEY_BRACE_LEFT, KEY_PRESSED);
                    hid_keyboard_button(KEY_BRACE_LEFT, KEY_RELEASED);
                    keyboard_state.last_pressed_key = KEY_NONE;
                } else {
                    hid_keyboard_button(k, mode);
                }
            } else {
                hid_keyboard_button(k, mode);
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

void layer_set(uint8_t layer)
{
    if (layer >= LAYERS_NUM || !layer_enabled[layer]) {
        return;
    }
    keyboard_state.layer = layer;
    trackball_load_layer_config();
    leds_blink(layer + 1, LEDS_BLINK_PERIOD_SHORT);
}

uint8_t layer_get(void)
{
    return keyboard_state.layer;
}

void fn_lock_set(uint8_t fn_lock) {
    keyboard_state.fn_lock = fn_lock ? 1 : 0;
    leds_blink(keyboard_state.fn_lock + 1, LEDS_BLINK_PERIOD_LONG);
}

void matrix_action(uint8_t row, uint8_t col, uint8_t mode)
{
    uint16_t k;
    uint8_t addr = row * MATRIX_COLS + col;
    uint8_t fn = keyboard_state.fn ? 1 : 0;

    if ((keyboard_state.mod_keys_on & KEY_CTRL_LEFT) && (keyboard_state.mod_keys_on & KEY_CTRL_RIGHT) && mode == KEY_PRESSED) {
        switch (addr)
        {
            case BUTTON_1:
                layer_set(0); hid_vendor_schedule_send(); return;
            case BUTTON_2:
                layer_set(1); hid_vendor_schedule_send(); return;
            case BUTTON_3:
                layer_set(2); hid_vendor_schedule_send(); return;
            case BUTTON_4:
                layer_set(3); hid_vendor_schedule_send(); return;
            case BUTTON_5:
                layer_set(4); hid_vendor_schedule_send(); return;
            case BUTTON_6:
                layer_set(5); hid_vendor_schedule_send(); return;
            case BUTTON_7:
                layer_set(6); hid_vendor_schedule_send(); return;
            case BUTTON_8:
                layer_set(7); hid_vendor_schedule_send(); return;
            case BUTTON_9:
                layer_set(8); hid_vendor_schedule_send(); return;
            case BUTTON_0:
                layer_set(9); hid_vendor_schedule_send(); return;
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
    if ((keyboard_state.mod_keys_on & KEY_CTRL_LEFT) 
        && (keyboard_state.mod_keys_on & KEY_CTRL_RIGHT) 
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

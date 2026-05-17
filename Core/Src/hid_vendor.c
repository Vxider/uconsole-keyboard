#include <stdint.h>
#include "hid_vendor.h"
#include "keymaps.h"
#include "usbd_custom_hid_if.h"

extern USBD_HandleTypeDef hUsbDeviceFS;
static uint8_t sending_scheduled = 0;

static uint8_t backlight_level_count(void) {
    return sizeof(backlight_vals) / sizeof(backlight_vals[0]);
}

void hid_vendor_on_recv(uint8_t* data) {
    if (data[0] == 0xFE && data[1] == 0x55 && data[2] == 0xAA) {
        // Bootloader magic
        jump_to_bootloader();
    }

    if ((data[0] & 0x0F) != 0x0F) {
        layer_set(data[0] & 0x0F);
    }

    for (int i = 0; i <= 8; i++) {
        if (data[2] & (1 << i)) {
            uint8_t value = (data[1] & (1 << i)) ? 1 : 0;
            switch (i) {
                case KEYBOARD_OPTION_FN_LOCK:
                    fn_lock_set(value);
                    break;
                case KEYBOARD_OPTION_BACKLIGHT: {
                    uint8_t backlight = (data[0] >> 4) & 0x0F;
                    if (backlight < backlight_level_count()) {
                        keyboard_state.backlight = backlight;
                    }
                    break;
                }
                case KEYBOARD_OPTION_DOUBLE_P_TO_BRACE_LEFT:
                    keyboard_state.double_p_to_brace_left = value;
                    break;
            }
        }
    }

    sending_scheduled = 1;
}

static USBD_StatusTypeDef hid_vendor_send_state(void) {
    uint8_t data[4] = {
        0x05,
        (uint8_t)(((keyboard_state.backlight & 0x0F) << 4) | (layer_get() & 0x0F)),
        keyboard_state.fn_lock ? (1 << KEYBOARD_OPTION_FN_LOCK) : 0,
        keyboard_state.double_p_to_brace_left ? (1 << KEYBOARD_OPTION_DOUBLE_P_TO_BRACE_LEFT) : 0
    };

    return hid_send_report(&hUsbDeviceFS, data, sizeof(data), 1);
}

void hid_vendor_schedule_send(void) {
    sending_scheduled = 1;
}

void hid_vendor_task(void) {
    if (sending_scheduled) {
        USBD_StatusTypeDef result = hid_vendor_send_state();
        if (result == USBD_OK) {
            sending_scheduled = 0;
        }
    }
}

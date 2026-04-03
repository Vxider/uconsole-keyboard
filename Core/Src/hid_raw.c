#include <stdint.h>
#include "hid_raw.h"
#include "keymaps.h"
#include "usbd_custom_hid_if.h"

void rawhid_on_recv(uint8_t* data) {
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
                case KEYBOARD_OPTION_DOUBLE_P_TO_BRACE_LEFT:
                    keyboard_state.double_p_to_brace_left = value;
                    break;
            }
        }
    }

    rawhid_send();
}

void rawhid_send() {
    uint8_t data[3] = {
        layer_get(),
        keyboard_state.fn_lock ? (1 << KEYBOARD_OPTION_FN_LOCK) : 0,
        keyboard_state.double_p_to_brace_left ? (1 << KEYBOARD_OPTION_DOUBLE_P_TO_BRACE_LEFT) : 0
    };
    Custom_HID_SendVendorValue(data);
}

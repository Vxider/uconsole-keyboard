#include <stdint.h>
#include "hid_raw.h"
#include "keymaps.h"
#include "usbd_custom_hid_if.h"

void rawhid_on_recv(uint8_t* data) {
    if (data[0] == 0xFF && data[1] == 0x55 && data[2] == 0xAA) {
        // Bootloader magic
        jump_to_bootloader();
    }

    if ((data[0] & 0x0F) != 0x0F) {
        layer_set(data[0] & 0x0F);
    }

    if ((data[0] & 0xF0) != 0xF0) {
        fn_lock_set(data[0] & 0x10 ? 1 : 0);
    }

    rawhid_send();
}

void rawhid_send() {
    uint8_t data[3] = {
        layer_get() | (keyboard_state.fn_lock ? 0x10 : 0),
        0x00, // Reserved
        0x00  // Reserved
    };
    Custom_HID_SendVendorValue(data);
    //hid_wait_for_usb_idle();
}

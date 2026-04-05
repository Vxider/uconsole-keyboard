#ifndef HID_RAW_H
#define HID_RAW_H

#include <stdint.h>

#define KEYBOARD_OPTION_FN_LOCK                     0
#define KEYBOARD_OPTION_DOUBLE_P_TO_BRACE_LEFT      7

void hid_vendor_on_recv(uint8_t* data);
uint8_t hid_vendor_send(void);

#endif

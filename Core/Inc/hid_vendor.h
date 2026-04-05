#ifndef HID_RAW_H
#define HID_RAW_H

#include <stdint.h>
#include "usbd_custom_hid_if.h"

#define KEYBOARD_OPTION_FN_LOCK                     0
#define KEYBOARD_OPTION_DOUBLE_P_TO_BRACE_LEFT      7

void hid_vendor_on_recv(uint8_t* data);
void hid_vendor_schedule_send(void);
void hid_vendor_task(void);

#endif

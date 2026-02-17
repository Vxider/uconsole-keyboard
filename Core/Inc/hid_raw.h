#ifndef HID_RAW_H
#define HID_RAW_H

#include <stdint.h>

void rawhid_on_recv(uint8_t* data);
void rawhid_send();

#endif

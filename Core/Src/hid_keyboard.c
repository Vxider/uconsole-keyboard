#include "hid_keyboard.h"
#include "usbd_custom_hid_if.h"

extern USBD_HandleTypeDef hUsbDeviceFS;

static uint8_t keyboard_report[9] = {0x01, 0, 0, 0, 0, 0, 0, 0, 0}; // Report ID 1 + 8 bytes data

static USBD_StatusTypeDef hid_keyboard_set_modifier(uint8_t modifier_bit)
{
    keyboard_report[1] |= modifier_bit;
    hid_wait_for_usb_idle();
    return USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, keyboard_report, 9);
}

static USBD_StatusTypeDef hid_keyboard_clear_modifier(uint8_t modifier_bit)
{
    keyboard_report[1] &= ~modifier_bit;
    hid_wait_for_usb_idle();
    return USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, keyboard_report, 9);
}

USBD_StatusTypeDef hid_keyboard_modifier(uint16_t modifier_bit, uint8_t mode)
{
    if (mode == KEY_PRESSED) {
        return hid_keyboard_set_modifier((uint8_t)modifier_bit);
    } else {
        return hid_keyboard_clear_modifier((uint8_t)modifier_bit);
    }
}

static USBD_StatusTypeDef hid_keyboard_press(uint8_t key)
{
    for (int i = 3; i < 9; i++) {
        if (keyboard_report[i] == 0) {
            keyboard_report[i] = key;
            break;
        }
    }
    
    hid_wait_for_usb_idle();
    return USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, keyboard_report, 9);
}

static USBD_StatusTypeDef hid_keyboard_release(uint8_t key)
{
    for (int i = 3; i < 9; i++) {
        if (keyboard_report[i] == key) {
            keyboard_report[i] = 0;
            break;
        }
    }
    
    hid_wait_for_usb_idle();
    return USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, keyboard_report, 9);
}

USBD_StatusTypeDef hid_keyboard_button(uint16_t key, uint8_t mode)
{
    if (mode == KEY_PRESSED) {
        return hid_keyboard_press((uint8_t)key);
    } else {
        return hid_keyboard_release((uint8_t)key);
    }
}

USBD_StatusTypeDef hid_keyboard_release_all(void)
{
    keyboard_report[0] = 0x01; // Report ID
    for (int i = 1; i < 9; i++) {
        keyboard_report[i] = 0;
    }
    
    hid_wait_for_usb_idle();
    return USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, keyboard_report, 9);
}


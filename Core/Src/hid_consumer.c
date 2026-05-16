#include "hid_consumer.h"
#include "hid_keyboard.h"  // For hid_wait_for_usb_idle()
#include "usbd_custom_hid_if.h"

extern USBD_HandleTypeDef hUsbDeviceFS;

// Current consumer control state (16-bit bit field)
static uint16_t consumer_state = 0;

// Convert 16-bit usage code to bit position in report
// This is a simplified mapping - only supports specific codes
static uint8_t usage_to_bit(uint16_t code)
{
    switch (code | CONSUMER_KEY_FLAG) {
        case CONSUMER_VOLUME_UP:      return 0;   // Bit 0
        case CONSUMER_VOLUME_DOWN:    return 1;   // Bit 1
        case CONSUMER_MUTE:           return 2;   // Bit 2
        case CONSUMER_BRIGHTNESS_UP:  return 3;   // Bit 3
        case CONSUMER_BRIGHTNESS_DOWN: return 4;  // Bit 4
        case CONSUMER_SCAN_NEXT_TRACK: return 5;  // Bit 5
        case CONSUMER_SCAN_PREV_TRACK: return 6;  // Bit 6
        case CONSUMER_STOP:           return 7;   // Bit 7
        case CONSUMER_PLAY_PAUSE:     return 8;   // Bit 8
        case CONSUMER_SCREEN_LOCK:    return 9;   // Bit 9
        default:                      return 255; // Invalid
    }
}

static USBD_StatusTypeDef hid_consumer_press(uint16_t code)
{
    uint8_t bit = usage_to_bit(code);
    if (bit == 255) return USBD_FAIL; // Invalid code
    
    uint16_t bit_mask = (uint16_t)(1 << bit);
    
    // Check if this bit is already set - don't send report if state hasn't changed
    if ((consumer_state & bit_mask) != 0) {
        return USBD_OK; // Already pressed, no need to send report
    }
    
    // Set the bit in current state
    consumer_state |= bit_mask;
    
    // Report ID (1 byte) + 16-bit bit field (2 bytes) = 3 bytes total
    uint8_t consumer_report[3];
    consumer_report[0] = 0x03; // Report ID
    consumer_report[1] = (uint8_t)(consumer_state & 0xFF);
    consumer_report[2] = (uint8_t)((consumer_state >> 8) & 0xFF);
    
    // Send press report
    return hid_send_report(&hUsbDeviceFS, consumer_report, 3, 1);
}

static int8_t hid_consumer_release(uint16_t code)
{
    if (code == 0) {
        // Release all buttons
        if (consumer_state == 0) {
            return USBD_OK; // Already released, no need to send report
        }
        consumer_state = 0;
    } else {
        // Release specific button
        uint8_t bit = usage_to_bit(code);
        if (bit == 255) return USBD_FAIL; // Invalid code
        
        uint16_t bit_mask = (uint16_t)(1 << bit);
        
        // Check if this bit is already cleared - don't send report if state hasn't changed
        if ((consumer_state & bit_mask) == 0) {
            return USBD_OK; // Already released, no need to send report
        }
        
        // Clear the bit in current state
        consumer_state &= ~bit_mask;
    }
    
    // Report ID (1 byte) + 16-bit bit field (2 bytes) = 3 bytes total
    uint8_t consumer_report[3];
    consumer_report[0] = 0x03; // Report ID
    consumer_report[1] = (uint8_t)(consumer_state & 0xFF);
    consumer_report[2] = (uint8_t)((consumer_state >> 8) & 0xFF);
    
    return hid_send_report(&hUsbDeviceFS, consumer_report, 3, 1);
}

int8_t hid_consumer_button(uint16_t code, uint8_t mode)
{
    code = code & ~CONSUMER_KEY_FLAG;
    if (mode == KEY_PRESSED) {
        return hid_consumer_press(code);
    } else {
        return hid_consumer_release(code);
    }
}

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// The values for the keyboard backlight PWM (maximum value is 2000)
static const uint16_t backlight_vals[3] = {0, 500, 2000};

// The initial value of the keyboard backlight (index of the backlight_vals array)
#define KEYBOARD_INITIAL_BACKLIGHT_VALUE_ID 1

// The time of inactivity in seconds after which the keyboard backlight will turn off
// 0 = never turn off
#define KEYBOARD_BACKLIGHT_OFF_TIME 15

// The duration of the keyboard backlight dim out effect in milliseconds
#define KEYBOARD_BACKLIGHT_DIM_OUT_DURATION 1000

// The value of the keyboard backlight when it is dimmed out
#define KEYBOARD_BACKLIGHT_DIMMED_OUT_VALUE 0

// If defined, the keyboard backlight will resume when the trackball is moved
// 0 = resume only on keypress, 1 = resume on keypress and trackball movement
#define KEYBOARD_BACKLIGHT_RESUME_BY_TRACKBALL 1

// If defined, the double semicolon will be replaced with an apostrophe
// This is useful for typing Russian text with the keyboard (жж -> э)
// or if you want to use your left hand to type quotes (;; -> ' and :: -> ")
// 0 = off, 1 = on
#define REPLACE_DOUBLE_SEMICOLON_WITH_APOSTROPHE 1

// If defined, the vertical scroll will be inverted
// 0 = traditional scrolling, 1 = natural scrolling
#define VERTICAL_SCROLL_INVERTED 0

// If defined, the horizontal scroll will be inverted
// 0 = traditional scrolling, 1 = natural scrolling
#define HORIZONTAL_SCROLL_INVERTED 0

// If the key is pressed and released within this time, the double press feature will be triggered
#define DOUBLE_PRESS_TIME_MS 300

#endif

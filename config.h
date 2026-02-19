#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// The values for the keyboard backlight PWM (maximum value is 2000)
static const uint16_t backlight_vals[3] = {0, 500, 2000};

// The initial value of the keyboard backlight (index of the backlight_vals array)
#define KEYBOARD_INITIAL_BACKLIGHT_VALUE_ID 0

// The time of inactivity in seconds after which the keyboard backlight will turn off
// 0 = never turn off
#define KEYBOARD_BACKLIGHT_OFF_TIME 15

// The duration of the keyboard backlight dim out effect in milliseconds
#define KEYBOARD_BACKLIGHT_DIM_OUT_DURATION 10000

// The value of the keyboard backlight when it is dimmed out
#define KEYBOARD_BACKLIGHT_DIMMED_OUT_VALUE 0

// If defined, the keyboard backlight will resume when the trackball is moved
// 0 = resume only on keypress, 1 = resume on keypress and trackball movement
#define KEYBOARD_BACKLIGHT_RESUME_BY_TRACKBALL 1

// Glider adds inertia: cursor keeps moving after you stop the trackball.
// 1 = on (cursor coasts to a stop), 0 = off (cursor stops instantly)
#define GLIDER_ENABLED 0

// How long (ms) the cursor moves at full speed before it starts slowing down.
// Higher = longer "slide" before braking. 0 = start braking immediately.
#define GLIDER_SUSTAIN_MAX_MS 10

// Scales sustain time by movement speed: sustain_ms = speed * this value.
// Higher = fast flicks coast longer. The result is capped by SUSTAIN_MAX_MS.
#define GLIDER_SUSTAIN_SPEED_SCALE 5.0f

// How fast the cursor slows down after sustain. Each millisecond, speed is
// multiplied by this value. Lower = stops faster, higher = slides longer.
// 0.85 ≈ stops in ~35ms, 0.90 ≈ ~55ms, 0.95 ≈ ~100ms, 0.80 ≈ ~25ms
#define GLIDER_DECAY_FACTOR_PER_MS 0.95f

// When speed drops below this, cursor stops completely. No need to change.
#define GLIDER_SPEED_EPSILON 0.01f

// If defined, the double semicolon will be replaced with an apostrophe
// This is useful for typing Russian text with the keyboard (жж -> э)
// or if you want to use your left hand to type quotes (;; -> ' and :: -> ")
// 0 = off, 1 = on
#define REPLACE_DOUBLE_SEMICOLON_WITH_APOSTROPHE 1

// If the key is pressed and released within this time, the double press feature will be triggered
#define DOUBLE_PRESS_TIME_MS 300

#endif

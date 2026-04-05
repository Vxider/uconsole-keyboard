#include "trackball.h"
#include "keyboard_state.h"
#include "keymaps.h"
#include "hid_mouse.h"
#include "ratemeter.h"
#include "math_utils.h"
#include "config.h"
#include "prec_time.h"
#include "main.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx.h"
#include <math.h>
#include <stdint.h>

typedef enum {
    AXIS_X,
    AXIS_Y,
    AXIS_NUM
} Axis;

static volatile int8_t distances[AXIS_NUM];
static RateMeter ratemeter[AXIS_NUM];
static float pointer_buffer[AXIS_NUM];
static int16_t wheel_buffer[AXIS_NUM]; /* Accumulates encoder ticks; int16_t to avoid overflow */
static bool as_wheel = false;
static bool lastWheelMode = false;

// Trackball configuration parameters for each layer
static float config_acceleration_exponent[LAYERS_NUM] = {0};
static float config_acceleration_divisor[LAYERS_NUM] = {0};
static float config_scroll_vertical_exponent[LAYERS_NUM] = {0};
static float config_scroll_vertical_divisor[LAYERS_NUM] = {0};
static float config_scroll_horizontal_exponent[LAYERS_NUM] = {0};
static float config_scroll_horizontal_divisor[LAYERS_NUM] = {0};
/* Scroll denominator is derived from divisor: divisor/50 so that one speed setting
   controls both sensitivity and "ticks per step" (filters accidental single touches). */
#define SCROLL_DIVISOR_TO_DENOMINATOR(divisor)  ((int)((divisor) / 50.0f) > 0 ? (int)((divisor) / 50.0f) : 1)

// Trackball configuration parameters for the current layer
static float acceleration_exponent;
static float acceleration_divisor;
static int scroll_vertical_denominator;
static float scroll_vertical_exponent;
static float scroll_vertical_divisor;
static int scroll_horizontal_denominator;
static float scroll_horizontal_exponent;
static float scroll_horizontal_divisor;

static int8_t apply_acceleration(int8_t steps, float rate, float exponent, float divisor)
{
    if (steps == 0) {
        return 0;
    }
    if (divisor <= 0.0f) {
        return steps;
    }
    const float factor = 1.0f + powf(rate, exponent) / divisor;
    const float scaled = fabsf((float)steps) * factor;
    const int8_t magnitude = clamp_int8((int32_t)roundf(scaled));
    return (steps > 0) ? magnitude : (int8_t)(-magnitude);
}

#if GLIDER_ENABLED
static uint16_t glider_sustain_from_speed(float speed)
{
    float s = fabsf(speed) * GLIDER_SUSTAIN_SPEED_SCALE;
    if (s <= 0) return 0;
    if (s >= (float)GLIDER_SUSTAIN_MAX_MS) return (uint16_t)GLIDER_SUSTAIN_MAX_MS;
    return (uint16_t)(s + 0.5f);
}
#endif

// Interrupt handlers
void trackball_interrupt_x_neg(void)
{
    distances[AXIS_X] -= 1;
    ratemeter_onInterrupt(&ratemeter[AXIS_X]);
}

void trackball_interrupt_x_pos(void)
{
    distances[AXIS_X] += 1;
    ratemeter_onInterrupt(&ratemeter[AXIS_X]);
}

void trackball_interrupt_y_neg(void)
{
    distances[AXIS_Y] -= 1;
    ratemeter_onInterrupt(&ratemeter[AXIS_Y]);
}

void trackball_interrupt_y_pos(void)
{
    distances[AXIS_Y] += 1;
    ratemeter_onInterrupt(&ratemeter[AXIS_Y]);
}

void trackball_task(void)
{
    int8_t x = 0, y = 0, w = 0, hw = 0;
    int8_t move_delta[AXIS_NUM];
    const uint32_t time_delta = PREC_TIME_DELTA_US();
    const float rate[AXIS_NUM] = {[AXIS_X] = ratemeter_rate(&ratemeter[AXIS_X]), [AXIS_Y] = ratemeter_rate(&ratemeter[AXIS_Y])};

    // Use wheel mode (Fn + trackball)
    as_wheel = keyboard_state.fn;

    if (time_delta == 0) {
        return; // Skip first iteration to get proper delta next time
    }
    
    ATOM_MOVE(move_delta[AXIS_X], distances[AXIS_X]);
    ATOM_MOVE(move_delta[AXIS_Y], distances[AXIS_Y]);

    // Reset wheel buffers only when switching modes
    if (as_wheel != lastWheelMode) {
        ratemeter_init(&ratemeter[AXIS_X]);
        ratemeter_init(&ratemeter[AXIS_Y]);
        wheel_buffer[AXIS_X] = 0;
        wheel_buffer[AXIS_Y] = 0;
        pointer_buffer[AXIS_X] = 0;
        pointer_buffer[AXIS_Y] = 0;
        move_delta[AXIS_X] = 0;
        move_delta[AXIS_Y] = 0;
        lastWheelMode = as_wheel;
    }

    ratemeter_tick(&ratemeter[AXIS_X], time_delta);
    ratemeter_tick(&ratemeter[AXIS_Y], time_delta);

    if (as_wheel) {        
        // Vertical scroll (wheel) - Y axis
        wheel_buffer[AXIS_Y] += move_delta[AXIS_Y];
        const int div_v = (scroll_vertical_denominator > 0) ? scroll_vertical_denominator : 1;
        const int32_t steps_v = wheel_buffer[AXIS_Y] / div_v;
        w = clamp_int8(steps_v);
        wheel_buffer[AXIS_Y] -= (int16_t)w * div_v;
        if (w != 0) {
            w = apply_acceleration(
                w,
                rate[AXIS_Y],
                scroll_vertical_exponent,
                scroll_vertical_divisor);
        }
        #if !VERTICAL_SCROLL_INVERTED
        w = -w;
        #endif
        
        // Horizontal scroll (pan) - X axis
        wheel_buffer[AXIS_X] += move_delta[AXIS_X];
        const int div_h = (scroll_horizontal_denominator > 0) ? scroll_horizontal_denominator : 1;
        const int32_t steps_h = wheel_buffer[AXIS_X] / div_h;
        hw = clamp_int8(steps_h);
        wheel_buffer[AXIS_X] -= (int16_t)hw * div_h;
        if (hw != 0) {
            hw = apply_acceleration(
                hw,
                rate[AXIS_X],
                scroll_horizontal_exponent,
                scroll_horizontal_divisor);
        }
        #if HORIZONTAL_SCROLL_INVERTED
        hw = -hw;
        #endif
    } else {
        // Pointer movement - X
        pointer_buffer[AXIS_X] += apply_acceleration(
            (int32_t)move_delta[AXIS_X],
            rate[AXIS_X],
            acceleration_exponent,
            acceleration_divisor);

        // Pointer movement - Y
        pointer_buffer[AXIS_Y] += apply_acceleration(
            (int32_t)move_delta[AXIS_Y],
            rate[AXIS_Y],
            acceleration_exponent,
            acceleration_divisor);

        x = (int8_t)clamp_int8((int32_t)pointer_buffer[AXIS_X]);
        y = (int8_t)clamp_int8((int32_t)pointer_buffer[AXIS_Y]);
        
        pointer_buffer[AXIS_X] -= (float)x;
        pointer_buffer[AXIS_Y] -= (float)y;
    }

    if (x != 0 || y != 0 || w != 0 || hw != 0) {
        hid_mouse_move(x, y, w, hw);
        #if KEYBOARD_BACKLIGHT_RESUME_BY_TRACKBALL
        keyboard_state.last_activity_time = HAL_GetTick();
        #endif
    }
}

void trackball_load_layer_config(void)
{
    #define LOAD_CONFIG(name, default_value) \
        name = config_##name[keyboard_state.layer]; \
        if (!name && keyboard_state.layer > 0) { \
            name = config_##name[0]; \
        } \
        if (!name) name = default_value;

    LOAD_CONFIG(acceleration_exponent, 1.0f + DEFAULT_TRACKBALL_ACCELERATION);
    LOAD_CONFIG(acceleration_divisor, 1000.0f / DEFAULT_TRACKBALL_SPEED);
    LOAD_CONFIG(scroll_vertical_exponent, 1.0f + DEFAULT_TRACKBALL_SCROLL_VERTICAL_ACCELERATION);
    LOAD_CONFIG(scroll_vertical_divisor, 10000.0f / DEFAULT_TRACKBALL_SCROLL_VERTICAL_SPEED);
    scroll_vertical_denominator = SCROLL_DIVISOR_TO_DENOMINATOR(scroll_vertical_divisor);
    LOAD_CONFIG(scroll_horizontal_exponent, 1.0f + DEFAULT_TRACKBALL_SCROLL_HORIZONTAL_ACCELERATION);
    LOAD_CONFIG(scroll_horizontal_divisor, 1000.0f / DEFAULT_TRACKBALL_SCROLL_HORIZONTAL_SPEED);
    scroll_horizontal_denominator = SCROLL_DIVISOR_TO_DENOMINATOR(scroll_horizontal_divisor);

    #undef LOAD_CONFIG
}

void trackball_set_acceleration(uint8_t layer, float value)
{
    config_acceleration_exponent[layer] = 1.0f + value;
}

void trackball_set_speed(uint8_t layer, float value)
{
    config_acceleration_divisor[layer] = 1000.0f / value;
}

void trackball_set_scroll_vertical_acceleration(uint8_t layer, float value)
{
    config_scroll_vertical_exponent[layer] = 1.0f + value;
}

void trackball_set_scroll_vertical_speed(uint8_t layer, float value)
{
    config_scroll_vertical_divisor[layer] = 10000.0f / value;
}

void trackball_set_scroll_horizontal_acceleration(uint8_t layer, float value)
{
    config_scroll_horizontal_exponent[layer] = 1.0f + value;
}

void trackball_set_scroll_horizontal_speed(uint8_t layer, float value)
{
    config_scroll_horizontal_divisor[layer] = 10000.0f / value;
}

void trackball_init(void)
{
    // Hall sensors are already configured as EXTI in MX_GPIO_Init
    ratemeter_init(&ratemeter[AXIS_X]);
    ratemeter_init(&ratemeter[AXIS_Y]);
    pointer_buffer[AXIS_X] = 0;
    pointer_buffer[AXIS_Y] = 0;
    distances[AXIS_X] = 0;
    distances[AXIS_Y] = 0;
    wheel_buffer[AXIS_X] = 0;
    wheel_buffer[AXIS_Y] = 0;
    as_wheel = false;
    lastWheelMode = false;

    trackball_load_layer_config();
}


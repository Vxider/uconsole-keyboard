#include "trackball.h"
#include "keyboard_state.h"
#include "keymaps.h"
#include "hid_mouse.h"
#include "ratemeter.h"
#include "glider.h"
#include "math_utils.h"
#include "config.h"
#include "main.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx.h"
#include <math.h>

typedef enum {
    AXIS_X,
    AXIS_Y,
    AXIS_NUM
} Axis;

static volatile int8_t distances[AXIS_NUM];
static RateMeter rateMeter[AXIS_NUM];
#if GLIDER_ENABLED
static Glider glider[AXIS_NUM];
#else
static float directError[AXIS_NUM];
static float directSpeed[AXIS_NUM];
static int8_t directDirection[AXIS_NUM];
static uint16_t directIdle[AXIS_NUM]; /* ms since last encoder tick */
#define DIRECT_STOP_MS 50             /* stop after this many ms without ticks */
#endif
static int16_t wheelBuffer;   /* Accumulates encoder ticks; int16_t to avoid overflow */
static int16_t hWheelBuffer;  /* Horizontal wheel buffer */
static bool asWheel = false;
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

static float rateToVelocityCurve(float input)
{
    // Power curve with exponent for smooth acceleration at all speeds
    // More gradual than quadratic, works well for both slow and fast movements    

    float rate = fabsf(input);
    return powf(rate, acceleration_exponent) / acceleration_divisor;
}

static int8_t applyScrollAcceleration(int8_t steps, float rate, float exponent, float divisor)
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
    __disable_irq();
    distances[AXIS_X] -= 1;
    ratemeter_onInterrupt(&rateMeter[AXIS_X]);
    if (!asWheel) {
        const float rx = ratemeter_rate(&rateMeter[AXIS_X]);
        const float ry = ratemeter_rate(&rateMeter[AXIS_Y]);
        const float rate = hypot_f(rx, ry);
        const float ratio = rateToVelocityCurve(rate) / (rate > 0.001f ? rate : 1.0f);
        const float vx = rx * ratio;
        const float vy = ry * ratio;
#if GLIDER_ENABLED
        glider_setDirection(&glider[AXIS_X], -1);
        glider_update(&glider[AXIS_X], vx, glider_sustain_from_speed(vx));
        glider_updateSpeed(&glider[AXIS_Y], vy);
#else
        directDirection[AXIS_X] = -1;
        directSpeed[AXIS_X] = vx;
        directSpeed[AXIS_Y] = vy;
        directIdle[AXIS_X] = 0;
        directIdle[AXIS_Y] = 0;
#endif
    }
    __enable_irq();
}

void trackball_interrupt_x_pos(void)
{
    __disable_irq();
    distances[AXIS_X] += 1;
    ratemeter_onInterrupt(&rateMeter[AXIS_X]);
    if (!asWheel) {
        const float rx = ratemeter_rate(&rateMeter[AXIS_X]);
        const float ry = ratemeter_rate(&rateMeter[AXIS_Y]);
        const float rate = hypot_f(rx, ry);
        const float ratio = rateToVelocityCurve(rate) / (rate > 0.001f ? rate : 1.0f);
        const float vx = rx * ratio;
        const float vy = ry * ratio;
#if GLIDER_ENABLED
        glider_setDirection(&glider[AXIS_X], 1);
        glider_update(&glider[AXIS_X], vx, glider_sustain_from_speed(vx));
        glider_updateSpeed(&glider[AXIS_Y], vy);
#else
        directDirection[AXIS_X] = 1;
        directSpeed[AXIS_X] = vx;
        directSpeed[AXIS_Y] = vy;
        directIdle[AXIS_X] = 0;
        directIdle[AXIS_Y] = 0;
#endif
    }
    __enable_irq();
}

void trackball_interrupt_y_neg(void)
{
    __disable_irq();
    distances[AXIS_Y] -= 1;
    ratemeter_onInterrupt(&rateMeter[AXIS_Y]);
    if (!asWheel) {
        const float rx = ratemeter_rate(&rateMeter[AXIS_X]);
        const float ry = ratemeter_rate(&rateMeter[AXIS_Y]);
        const float rate = hypot_f(rx, ry);
        const float ratio = rateToVelocityCurve(rate) / (rate > 0.001f ? rate : 1.0f);
        const float vx = rx * ratio;
        const float vy = ry * ratio;
#if GLIDER_ENABLED
        glider_setDirection(&glider[AXIS_Y], -1);
        glider_updateSpeed(&glider[AXIS_X], vx);
        glider_update(&glider[AXIS_Y], vy, glider_sustain_from_speed(vy));
#else
        directDirection[AXIS_Y] = -1;
        directSpeed[AXIS_X] = vx;
        directSpeed[AXIS_Y] = vy;
        directIdle[AXIS_X] = 0;
        directIdle[AXIS_Y] = 0;
#endif
    }
    __enable_irq();
}

void trackball_interrupt_y_pos(void)
{
    __disable_irq();
    distances[AXIS_Y] += 1;
    ratemeter_onInterrupt(&rateMeter[AXIS_Y]);
    if (!asWheel) {
        const float rx = ratemeter_rate(&rateMeter[AXIS_X]);
        const float ry = ratemeter_rate(&rateMeter[AXIS_Y]);
        const float rate = hypot_f(rx, ry);
        const float ratio = rateToVelocityCurve(rate) / (rate > 0.001f ? rate : 1.0f);
        const float vx = rx * ratio;
        const float vy = ry * ratio;
#if GLIDER_ENABLED
        glider_setDirection(&glider[AXIS_Y], 1);
        glider_updateSpeed(&glider[AXIS_X], vx);
        glider_update(&glider[AXIS_Y], vy, glider_sustain_from_speed(vy));
#else
        directDirection[AXIS_Y] = 1;
        directSpeed[AXIS_X] = vx;
        directSpeed[AXIS_Y] = vy;
        directIdle[AXIS_X] = 0;
        directIdle[AXIS_Y] = 0;
#endif
    }
    __enable_irq();
}

void trackball_task(void)
{
    static uint32_t last_time = 0;
    uint32_t current_time = HAL_GetTick();
    uint8_t delta = 1;
    
    if (last_time != 0) {
        uint32_t time_delta = current_time - last_time;
        // Clamp delta to reasonable range (1-255 ms, uint8_t limit)
        if (time_delta == 0) {
            time_delta = 1; // Minimum 1 ms
        } else if (time_delta > 255) {
            time_delta = 255; // Maximum uint8_t
        }
        delta = (uint8_t)time_delta;
    } else {
        // First call - initialize last_time
        last_time = current_time;
        return; // Skip first iteration to get proper delta next time
    }
    last_time = current_time;
    
    int8_t x = 0, y = 0, w = 0, hw = 0;
    
    __disable_irq();
    // Use wheel mode (Fn + trackball)
    asWheel = keyboard_state.fn;
    
    // Reset wheel buffers only when switching modes
    if (asWheel != lastWheelMode) {
        ratemeter_init(&rateMeter[AXIS_X]);
        ratemeter_init(&rateMeter[AXIS_Y]);
        wheelBuffer = 0;
        hWheelBuffer = 0;
        lastWheelMode = asWheel;
    }
    
    if (asWheel) {        
        ratemeter_tick(&rateMeter[AXIS_X], delta);
        ratemeter_tick(&rateMeter[AXIS_Y], delta);

        // Vertical scroll (wheel) - Y axis
        wheelBuffer += distances[AXIS_Y];
        const int div_v = (scroll_vertical_denominator > 0) ? scroll_vertical_denominator : 1;
        const int32_t steps_v = wheelBuffer / div_v;
        w = clamp_int8(steps_v);
        wheelBuffer -= (int16_t)w * div_v;
        if (w != 0) {
            const float scrollRate = ratemeter_rate(&rateMeter[AXIS_Y]);
            w = applyScrollAcceleration(
                w,
                scrollRate,
                scroll_vertical_exponent,
                scroll_vertical_divisor);
        }
        
        // Horizontal scroll (pan) - X axis
        hWheelBuffer += distances[AXIS_X];
        const int div_h = (scroll_horizontal_denominator > 0) ? scroll_horizontal_denominator : 1;
        const int32_t steps_h = hWheelBuffer / div_h;
        hw = clamp_int8(steps_h);
        hWheelBuffer -= (int16_t)hw * div_h;
        if (hw != 0) {
            const float panRate = ratemeter_rate(&rateMeter[AXIS_X]);
            hw = applyScrollAcceleration(
                hw,
                panRate,
                scroll_horizontal_exponent,
                scroll_horizontal_divisor);
        }

        x = 0;  // No X movement in wheel mode
        y = 0;  // No Y movement in wheel mode
    } else {
        ratemeter_tick(&rateMeter[AXIS_X], delta);
        ratemeter_tick(&rateMeter[AXIS_Y], delta);
#if GLIDER_ENABLED
        GlideResult rX = glider_glide(&glider[AXIS_X], delta);
        GlideResult rY = glider_glide(&glider[AXIS_Y], delta);
        x = rX.value;
        y = rY.value;
        if (rX.stopped) {
            glider_stop(&glider[AXIS_X]);
        }
        if (rY.stopped) {
            glider_stop(&glider[AXIS_Y]);
        }
#else
        /* Direct mode: speed & direction are set in interrupts (fresh rate).
           Idle timer counts ms without ticks; stop after DIRECT_STOP_MS. */
        for (int a = 0; a < AXIS_NUM; a++) {
            uint16_t newIdle = directIdle[a] + delta;
            if (newIdle > DIRECT_STOP_MS) newIdle = DIRECT_STOP_MS;
            directIdle[a] = newIdle;
            if (directIdle[a] >= DIRECT_STOP_MS) {
                directSpeed[a] = 0;
                directError[a] = 0;
            }
        }

        directError[AXIS_X] += (float)directDirection[AXIS_X] * directSpeed[AXIS_X] * (float)delta;
        directError[AXIS_Y] += (float)directDirection[AXIS_Y] * directSpeed[AXIS_Y] * (float)delta;

        x = (int8_t)clamp_int8((int32_t)directError[AXIS_X]);
        y = (int8_t)clamp_int8((int32_t)directError[AXIS_Y]);
        directError[AXIS_X] -= (float)x;
        directError[AXIS_Y] -= (float)y;
#endif
    }
    
    distances[AXIS_X] = 0;
    distances[AXIS_Y] = 0;
    __enable_irq();
    
    if (asWheel) {
        // In wheel mode, use pan for horizontal scroll
        if (w != 0 || hw != 0) {
            hid_mouse_move_with_pan(0, 0, -w, hw);  // Invert horizontal scroll direction
            #if KEYBOARD_BACKLIGHT_RESUME_BY_TRACKBALL
            keyboard_state.last_activity_time = HAL_GetTick();
            #endif
        }
    } else {
        if (x != 0 || y != 0 || w != 0) {
            hid_mouse_move(x, y, -w);
            #if KEYBOARD_BACKLIGHT_RESUME_BY_TRACKBALL
            keyboard_state.last_activity_time = HAL_GetTick();
            #endif
        }
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
    LOAD_CONFIG(acceleration_divisor, 10000.0f / DEFAULT_TRACKBALL_SPEED);
    LOAD_CONFIG(scroll_vertical_exponent, 1.0f + DEFAULT_TRACKBALL_SCROLL_VERTICAL_ACCELERATION);
    LOAD_CONFIG(scroll_vertical_divisor, 10000.0f / DEFAULT_TRACKBALL_SCROLL_VERTICAL_SPEED);
    scroll_vertical_denominator = SCROLL_DIVISOR_TO_DENOMINATOR(scroll_vertical_divisor);
    LOAD_CONFIG(scroll_horizontal_exponent, 1.0f + DEFAULT_TRACKBALL_SCROLL_HORIZONTAL_ACCELERATION);
    LOAD_CONFIG(scroll_horizontal_divisor, 10000.0f / DEFAULT_TRACKBALL_SCROLL_HORIZONTAL_SPEED);
    scroll_horizontal_denominator = SCROLL_DIVISOR_TO_DENOMINATOR(scroll_horizontal_divisor);

    #undef LOAD_CONFIG
}

void trackball_set_acceleration(uint8_t layer, float value)
{
    config_acceleration_exponent[layer] = 1.0f + value;
}

void trackball_set_speed(uint8_t layer, float value)
{
    config_acceleration_divisor[layer] = 10000.0f / value;
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
    ratemeter_init(&rateMeter[AXIS_X]);
    ratemeter_init(&rateMeter[AXIS_Y]);
#if GLIDER_ENABLED
    glider_init(&glider[AXIS_X]);
    glider_init(&glider[AXIS_Y]);
#else
    directError[AXIS_X] = 0;
    directError[AXIS_Y] = 0;
    directSpeed[AXIS_X] = 0;
    directSpeed[AXIS_Y] = 0;
    directDirection[AXIS_X] = 0;
    directDirection[AXIS_Y] = 0;
    directIdle[AXIS_X] = DIRECT_STOP_MS;
    directIdle[AXIS_Y] = DIRECT_STOP_MS;
#endif
    distances[AXIS_X] = 0;
    distances[AXIS_Y] = 0;
    wheelBuffer = 0;
    hWheelBuffer = 0;
    asWheel = false;
    lastWheelMode = false;

    trackball_load_layer_config();
}


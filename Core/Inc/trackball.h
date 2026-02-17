#ifndef TRACKBALL_H
#define TRACKBALL_H

#include <stdint.h>
#include "main.h"

#define DEFAULT_TRACKBALL_SPEED 50
#define DEFAULT_TRACKBALL_ACCELERATION 0.3f
#define DEFAULT_TRACKBALL_SCROLL_VERTICAL_SPEED 100
#define DEFAULT_TRACKBALL_SCROLL_VERTICAL_ACCELERATION 0.3f
#define DEFAULT_TRACKBALL_SCROLL_HORIZONTAL_SPEED 100
#define DEFAULT_TRACKBALL_SCROLL_HORIZONTAL_ACCELERATION 0.3f

void trackball_init(void);
void trackball_task(void);
void trackball_interrupt_x_neg(void);
void trackball_interrupt_x_pos(void);
void trackball_interrupt_y_neg(void);
void trackball_interrupt_y_pos(void);
void trackball_load_layer_config(void);
void trackball_set_acceleration(uint8_t layer, float value);
void trackball_set_speed(uint8_t layer, float value);
void trackball_set_scroll_vertical_acceleration(uint8_t layer, float value);
void trackball_set_scroll_vertical_speed(uint8_t layer, float value);
void trackball_set_scroll_horizontal_acceleration(uint8_t layer, float value);
void trackball_set_scroll_horizontal_speed(uint8_t layer, float value);

#endif


#ifndef PREC_TIME_H
#define PREC_TIME_H

#include <stdint.h>

void prec_time_tick(void);
uint64_t prec_time_get_us(void);
void prec_time_wait_us(uint32_t time_us);

#define PREC_TIME_DELTA_US() \
    ({ \
        static uint64_t last_time = 0; \
        const uint64_t now = prec_time_get_us(); \
        const uint32_t delta = last_time ? now - last_time : 0; \
        last_time = now; \
        delta; \
    })

static inline uint32_t prec_time_delta_us(uint64_t *last_time)
{
    const uint64_t now = prec_time_get_us();
    const uint32_t delta = *last_time ? now - *last_time : 0;
    *last_time = now;
    return delta;
}

#endif
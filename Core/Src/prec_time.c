#include "main.h"
#include "prec_time.h"

static volatile uint32_t prec_time_ms_value = 0;

void prec_time_tick(void)
{
    prec_time_ms_value++;
}

/* Get the current time in microseconds */
uint64_t prec_time_get_us(void)
{
    const uint32_t value_pre = prec_time_ms_value;
    const uint32_t value_us = htim2.Instance->CNT;
    const uint32_t value_post = prec_time_ms_value;

    /* Check if the prec_time_ms_value is valid or it was changed during the measurement */
    if (value_pre == value_post || value_us > 500) {
        /* The prec_time_ms_value is valid or it was changed after reading the timer value */
        return (uint64_t)value_pre * 1000 + value_us;
    } else {
        /* The prec_time_ms_value was changed during the measurement, so we need to use the next millisecond */
        return (uint64_t)(value_pre + 1) * 1000 + value_us;
    }
}

/* Wait for a given number of microseconds */
void prec_time_wait_us(uint32_t time_us)
{
    const uint64_t start_time = prec_time_get_us();
    while ((uint32_t)(prec_time_get_us() - start_time) < time_us) {
        /* Wait for the time to pass */
    }
}

#include "ratemeter.h"
#include "stm32f1xx_hal.h"
#include "prec_time.h"

void ratemeter_init(RateMeter* rm)
{
    rm->lastTime = 0;
    rm->averageDelta = 0;
    rm->timeout = 0;
}

void ratemeter_onInterrupt(RateMeter* rm)
{
    const uint64_t now = prec_time_get_us();
    if (rm->timeout == 0) {
        rm->averageDelta = CUTOFF_US;
    } else {
        uint64_t delta = prec_time_delta_us(&rm->lastTime);
        if (delta > CUTOFF_US) {
            delta = CUTOFF_US;
        }
        rm->averageDelta = (rm->averageDelta + delta) / 2;
    }
    rm->lastTime = now;
    rm->timeout = CUTOFF_US;
}

void ratemeter_tick(RateMeter* rm, uint32_t delta)
{
    if (rm->timeout > delta) {
        rm->timeout -= delta;
    } else {
        rm->timeout = 0;
    }
    if (rm->timeout != 0) {
        rm->averageDelta += delta;
    }
}

void ratemeter_expire(RateMeter* rm)
{
    rm->timeout = 0;
}

uint16_t ratemeter_delta(const RateMeter* rm)
{
    return rm->averageDelta;
}

float ratemeter_rate(const RateMeter* rm)
{
    if (rm->timeout == 0) {
        return 0.0f;
    } else if (rm->averageDelta == 0) {
        return CUTOFF_US;
    } else {
        return CUTOFF_US * 1.0f / (float)rm->averageDelta;
    }
}


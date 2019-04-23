#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdint.h>
#include "stm32f0xx_hal.h"

static inline uint32_t disable_irq(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static inline void enable_irq(uint32_t priority_mask)
{
    __set_PRIMASK(priority_mask);
}

void hex32(char *out, uint32_t val);

#endif

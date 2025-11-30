#pragma once

#include <stdint.h>

enum
{
    LAPIC_REG_ID        = 0x020,
    LAPIC_REG_EOI       = 0x0B0,
    LAPIC_REG_SVR       = 0x0F0,
    LAPIC_REG_LVT_TIMER = 0x320,
    LAPIC_REG_INIT_CNT  = 0x380,
    LAPIC_REG_CUR_CNT   = 0x390,
    LAPIC_REG_DIVIDE    = 0x3E0,
};

#define LAPIC_TIMER_MODE_ONE_SHOT (0u << 17)

#define LAPIC_TIMER_MODE_PERIODIC (1u << 17)

#define LAPIC_TIMER_VECTOR 0x20

#define LAPIC_TIMER_DIVIDE_16 0x3

void lapic_init(uint64_t lapic_base_phys);

void lapic_eoi(void);

void lapic_timer_init(uint32_t hz);

void lapic_timer_tick(void);

void lapic_sleep_msec(uint64_t ms);
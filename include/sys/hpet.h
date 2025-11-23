#pragma once

#include <stdint.h>
#include <stdbool.h>

void     hpet_init(void);
bool     hpet_is_available(void);
uint64_t hpet_counter(void);
uint64_t hpet_get_nsec(void);
uint64_t hpet_get_msec(void);
void     hpet_sleep_msec(uint64_t ms);
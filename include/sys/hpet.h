#pragma once

#include <stdint.h>

void hpet_init(void);
void hpet_sleep_msec(uint64_t ms);
uint64_t hpet_get_freq_hz(void);
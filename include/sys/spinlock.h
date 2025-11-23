#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    volatile uint32_t v;
} spinlock_t;

#define SPINLOCK_INIT { .v = 0 }

void spinlock_init(spinlock_t *lock);

bool spin_lock(spinlock_t *lock);
void spin_unlock(spinlock_t *lock);


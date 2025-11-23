#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <system.h>

typedef struct
{
    volatile int32_t y;
} atomic_int32_t;

static inline void atomic_store_relaxed32(volatile int32_t *ptr, int32_t value)
{
    __atomic_store_n(ptr, value, __ATOMIC_RELAXED);
}

static inline void atomic_store_release32(volatile int32_t *ptr, int32_t value)
{
    __atomic_store_n(ptr, value, __ATOMIC_RELEASE);
}

static inline int32_t atomic_load_relaxed32(volatile int32_t *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_RELAXED);
}

static inline int32_t atomic_load_acquire32(volatile int32_t *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_ACQUIRE);
}

static inline int32_t atomic_fetch_add32(volatile int32_t *ptr, int32_t delta)
{
    return __atomic_fetch_add(ptr, delta, __ATOMIC_ACQ_REL);
}

static inline int32_t atomic_fetch_sub32(volatile int32_t *ptr, int32_t delta)
{
    return __atomic_fetch_sub(ptr, delta, __ATOMIC_ACQ_REL);
}

static inline int32_t atomic_inc32(volatile int32_t *ptr)
{
    return __atomic_add_fetch(ptr, 1, __ATOMIC_ACQ_REL);
}

static inline int32_t atomic_dec32(volatile int32_t *ptr)
{
    return __atomic_sub_fetch(ptr, 1, __ATOMIC_ACQ_REL);
}

static inline bool atomic_cas32(volatile int32_t *ptr,
                                int32_t *expected,
                                int32_t desired)
{
    return __atomic_compare_exchange_n(ptr,
                                       expected,
                                       desired,
                                       /* weak = */ false,
                                       __ATOMIC_ACQ_REL,
                                       __ATOMIC_ACQUIRE);
}

static inline int32_t atomic_exchange32(volatile int32_t *ptr, int32_t value)
{
    return __atomic_exchange_n(ptr, value, __ATOMIC_ACQ_REL);
}
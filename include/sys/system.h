#pragma once
#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>
#include <stddef.h>
#include <stdnoreturn.h>
#include <console.h>
#include <memstring.h>

#define KERNEL_NAME     "LAIN"
#define KERNEL_VER      "0.13.37"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define UNUSED(X) (void)(x)

/**
 * gnu defs
 */
#define PACKED __attribute__((packed))
#define ALIGNED(x) __attribute__((aligned(x)))
#define NORETURN _Noreturn
#define ALWAYS_INLINE inline __attribute__((always_inline))
#define NOINLINE __attribute__((noinline))
#define USED_SECTION(sec) __attribute__((used, section(sec)))

typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL
} log_level_t;

void kputc(char c);
void kprint(const char *s);
void kprintf(const char *fmt, ...);
void klog(log_level_t level, const char *fmt, ...);

NORETURN void panic(const char *msg);
NORETURN void halt(void);

static ALWAYS_INLINE uint64_t rdtsc(void) 
{
    uint32_t lo, hi;
    asm volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

static ALWAYS_INLINE void breakpoint(void) 
{
    asm volatile ("int3");
}

static ALWAYS_INLINE void cpu_pause(void) 
{
    asm volatile ("pause");
}

#endif
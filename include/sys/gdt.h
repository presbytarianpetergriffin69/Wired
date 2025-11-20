#pragma once

#include <stdint.h>
#include <system.h>

typedef struct PACKED gdt_descriptor
{
    uint16_t limit;
    uint64_t base;
} gdt_descriptor_t;

void gdt_init(void);
#pragma once

#include <stdint.h>

struct idt_entry
{
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} PACKED;

struct idt_descriptor
{
    uint16_t limit;
    uint64_t base;
} PACKED;

void idt_init(void);
void idt_set_gate(int vec, void (*handler)(void), uint8_t ist, uint8_t flags);
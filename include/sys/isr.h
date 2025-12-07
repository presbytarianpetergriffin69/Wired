#pragma once
#include <stdint.h>

typedef struct isr_regs 
{
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no;
    uint64_t err_code;
    uint64_t rip, cs, rflags, rsp, ss;
} isr_regs_t;

typedef void (*irq_handler_t)(isr_regs_t *r, void *ctx);

void isr_dispatch(isr_regs_t *r);
void irq_register(uint8_t vec, irq_handler_t handler, void *ctx);
void irq_unregister(uint8_t vec);
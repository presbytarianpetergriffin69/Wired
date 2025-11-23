#include <stdint.h>
#include <system.h>
#include <idt.h>
#include <isr.h>

extern void idt_load(struct idt_descriptor *idtr);

extern void isr_default_stub(void);

extern void (*isr_stub_table[])(void);

#define IDT_ENTRIES 256

static struct idt_entry idt[IDT_ENTRIES];
static struct idt_descriptor idtr;

void idt_set_gate(int vec, void (*handler)(void), uint8_t ist, uint8_t flags)
{
    uint64_t addr = (uint64_t)handler;

    idt[vec].offset_low  = (uint16_t)(addr & 0xFFFF);
    idt[vec].selector    = 0x08;       // kernel code segment
    idt[vec].ist         = ist & 0x7;  // lower 3 bits 
    idt[vec].type_attr   = flags; 
    idt[vec].offset_mid  = (uint16_t)((addr >> 16) & 0xFFFF);
    idt[vec].offset_high = (uint32_t)((addr >> 32) & 0xFFFFFFFF);
    idt[vec].zero        = 0;
}

void idt_init(void)
{
    idtr.limit = sizeof(idt) - 1;
    idtr.base = (uint64_t)&idt;

    for (int i = 0; i < 32; i++) {
        idt_set_gate(i, isr_stub_table[i], 1, 0x8E);
    }

    idt_load(&idtr);
}
#include <stdint.h>
#include <system.h>
#include <idt.h>

extern void idt_load(struct idt_descriptor *idtr);

extern void isr_default_stub(void);

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
    for (int i = 0; i < IDT_ENTRIES; i++)
    {
        idt[i].offset_low  = 0;
        idt[i].selector    = 0;
        idt[i].ist         = 0;
        idt[i].type_attr   = 0;
        idt[i].offset_mid  = 0;
        idt[i].offset_high = 0;
        idt[i].zero        = 0;
    }

    for (int i = 0; i < 32; i++)
    {
        idt_set_gate(i, isr_default_stub, 0, 0x8E);
    }

    idtr.base = (uint64_t)&idt[0];
    idtr.limit = (uint16_t)(sizeof(idt) - 1);

    idt_load(&idtr);
}
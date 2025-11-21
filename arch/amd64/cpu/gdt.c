#include <stdint.h>

#include <system.h>
#include <gdt.h>
#include <tss.h>

struct gdt_entry
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} PACKED;

struct gdt_tss_entry
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
    uint32_t base_upper;
    uint32_t reserved;
} PACKED;

struct gdt_table
{
    struct gdt_entry     entries[5];
    struct gdt_tss_entry tss;
} PACKED;

/**
 * gdt layout
 * 0000 null descriptor segment
 * 0008 kernel code segment
 * 0010 kernel data segment
 * 0018 user mode code segment
 * 0020 user mode data segment
 * 0028 tss segment
 */

static struct gdt_table gdt_table;
static struct gdt_descriptor gdtr;

extern void gdt_load(struct gdt_descriptor *gdtr);
extern void tss_load(uint16_t selector);

static void gdt_set_entry(int idx, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags)
{
    struct gdt_entry *e = &gdt_table.entries[idx];

    e->limit_low = (uint16_t)(limit & 0xFFFF);
    e->base_low = (uint16_t)(base & 0xFFFF);
    e->base_mid = (uint8_t)((base >> 16) & 0xFF);
    e->access = access;
    e->granularity = (uint8_t)(((limit >> 16) & 0x0F) | (flags & 0xF0));
    e->base_high = (uint8_t)((base >> 24) & 0xFF);
}

void gdt_init(void)
{
    kprintf("[GDT] Setting various entries\n");

    // NULL DESCRIPTOR
    gdt_set_entry(0, 0, 0, 0, 0);

    // kernel code segment
    gdt_set_entry(1, 0, 0, 0x9A, 0x20);

    // kernel data segment
    gdt_set_entry(2, 0, 0, 0x92, 0x00);

    // user mode code segment
    gdt_set_entry(3, 0, 0, 0xF2, 0x00);

    // user mode data segment
    gdt_set_entry(4, 0, 0, 0xFA, 0x20);

    // tss descriptor (last in mem order)

    uint64_t base = (uint64_t)&g_tss;
    uint32_t limit = sizeof(struct tss64) - 1;

    gdt_table.tss.limit_low = (uint16_t)(limit & 0xFFFF);
    gdt_table.tss.base_low = (uint16_t)(base & 0xFFFF);
    gdt_table.tss.base_mid = (uint8_t)((base >> 16) & 0xFF);
    gdt_table.tss.access = 0x89;
    gdt_table.tss.granularity = (uint8_t)(((limit >> 16) & 0x0F));
    gdt_table.tss.base_high = (uint8_t)((base >> 24) & 0xFF);
    gdt_table.tss.base_upper = (uint32_t)(base >> 32);
    gdt_table.tss.reserved = 0;

    gdtr.base = (uint64_t)&gdt_table;
    gdtr.limit = sizeof(gdt_table) - 1;

    kprintf("[GDT] Loading GDT\n");

    gdt_load(&gdtr);

    tss_load(0x28);
}
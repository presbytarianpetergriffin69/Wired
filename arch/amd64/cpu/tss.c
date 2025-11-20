#include <stdint.h>
#include <stddef.h>

#include <system.h>
#include <tss.h>

struct tss64 g_tss;

void tss_init(void) 
{
    for (size_t i = 0; i < sizeof(g_tss); i++)
    {
        ((uint8_t *)&g_tss)[i] = 0;
    }

    g_tss.iomap_base = sizeof(struct tss64);
}

void tss_set_rsp0(uint64_t rsp) 
{
    g_tss.rsp0 = rsp;
}

static uint8_t bootstrap_stack[4096] ALIGNED(16);

uint64_t bootstrap_stack_top(void)
{
    return (uint64_t)(bootstrap_stack + sizeof(bootstrap_stack));
}


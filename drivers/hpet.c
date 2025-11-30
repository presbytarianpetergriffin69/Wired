#include <stdint.h>
#include <system.h>
#include <acpi.h>
#include <hpet.h>

#define HPET_REG_GCAP_ID    0x000
#define HPET_REG_GEN_CONF   0x010
#define HPET_REG_MAIN_CNT   0x0F0

static volatile uint64_t *hpet_mmio = NULL;
static uint64_t hpet_period_fs = 0;
static uint64_t hpet_ticks_per_ms = 0;

static inline uint64_t hpet_read(uint32_t reg)
{
    return hpet_mmio[reg / 8];
}

static inline void hpet_write(uint32_t reg, uint64_t val)
{
    hpet_mmio[reg / 8] = val;
}

void hpet_init(void)
{
    uint64_t base = acpi_get_hpet_base();
    if (!base) {
        kprintf("[HPET] No HPET base from ACPI\n");
        return;
    }

    hpet_mmio = (volatile uint64_t *)ACPI_PHYS_TO_VIRT(base);

    uint64_t gcap = hpet_read(HPET_REG_GCAP_ID);
    hpet_period_fs = gcap >> 32;

    if (!hpet_period_fs) {
        kprintf("[HPET] Invalid Period (0)\n");
        hpet_mmio = NULL;
        return;
    }

    hpet_ticks_per_ms = 1000000000000ULL / hpet_period_fs;

    uint64_t cfg = hpet_read(HPET_REG_GEN_CONF);
    cfg &= ~1ULL;
    hpet_write(HPET_REG_GEN_CONF, cfg);

    hpet_write(HPET_REG_MAIN_CNT, 0);

    cfg |= 1ULL;
    hpet_write(HPET_REG_GEN_CONF, cfg);

    kprintf("[HPET] base=0x%016llx period=%llufs ticks/ms=%llu\n",
            (unsigned long long)base,
            (unsigned long long)hpet_period_fs,
            (unsigned long long)hpet_ticks_per_ms);
}

uint64_t hpet_get_freq_hz(void)
{
    if (!hpet_period_fs) return 0;
    return 1000000000000000ULL / hpet_period_fs;
}

void hpet_sleep_msec(uint64_t ms)
{
    if (!hpet_mmio || hpet_ticks_per_ms == 0)
        return;

    uint64_t ticks = ms * hpet_ticks_per_ms;
    uint64_t start = hpet_read(HPET_REG_MAIN_CNT);

    while ((hpet_read(HPET_REG_MAIN_CNT) - start) < ticks)
        __asm__ volatile("pause");
} 
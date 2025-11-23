#include <stdint.h>
#include <stdbool.h>
#include <system.h>
#include <acpi.h> // hpet table defined here
#include <hpet.h>

#define HPET_REG_CAP_ID 0x000
#define HPET_REG_GEN_CONFIG 0x010
#define HPET_REG_GEN_INT_STATUS 0x020
#define HPET_REG_MAIN_COUNTER 0x0F0

#define HPET_T0_CONFIG 0x100
#define HPET_T0_COMPARATOR 0x108
#define HPET_T0_FSB_ROUTE 0x110

static volatile uint64_t *hpet_mmio = NULL;
static uint64_t hpet_period_fs = 0;   // femtoseconds per tick
static uint64_t hpet_freq_hz = 0;   // ticks per second
static uint64_t hpet_ticks_per_ms = 0; // ticks per millisecond
static bool hpet_available = false;

static inline uint64_t hpet_read(uint32_t reg)
{
    return *(volatile uint64_t *)((uintptr_t)hpet_mmio + reg);
}

static inline void hpet_write(uint32_t reg, uint64_t val)
{
    *(volatile uint64_t *)((uintptr_t)hpet_mmio + reg);
}

void hpet_init(void)
{
    ACPISDTHeader *hdr = acpi_find_table("HPET");
    if (!hdr) {
        kprintf("ACPI HPET table not found\n");
        return;
    }

    struct acpi_hpet *tbl = (struct acpi_hpet *)hdr;
    uint64_t phys_base = tbl->base_address.address;

    hpet_mmio = (volatile uint64_t *)ACPI_PHYS_TO_VIRT(phys_base);

    uint64_t cap = hpet_read(HPET_REG_CAP_ID);

    hpet_period_fs = cap & 0xFFFFFFFFu;
    if (!hpet_period_fs) {
        kprintf("HPET invalid period (0fs)\n");
        hpet_available = false;
        return;
    }

    hpet_freq_hz = 1000000000000000ULL / hpet_period_fs;
    if (!hpet_freq_hz) {
        kprintf("HPET: computed zero frequency, disabling\n");
        hpet_available = false;
        return;
    }

    hpet_ticks_per_ms = hpet_freq_hz / 1000ULL;
    if (!hpet_ticks_per_ms) {
        hpet_ticks_per_ms = 1; // worst case, very slow HPET
    }

    kprintf("HPET: base=%p, period=%llu fs, freq=%llu Hz, ticks/ms=%llu\n",
            hpet_mmio,
            (unsigned long long)hpet_period_fs,
            (unsigned long long)hpet_freq_hz,
            (unsigned long long)hpet_ticks_per_ms);

    uint64_t cfg = hpet_read(HPET_REG_GEN_CONFIG);
    cfg &= ~1ULL;
    hpet_write(HPET_REG_GEN_CONFIG, cfg);

    hpet_write(HPET_REG_MAIN_COUNTER, 0);

    hpet_write(HPET_REG_GEN_INT_STATUS, hpet_read(HPET_REG_GEN_INT_STATUS));

    cfg |= 1ULL;
    hpet_write(HPET_REG_GEN_CONFIG, cfg);

    hpet_available = true;
}

bool hpet_is_available(void)
{
    return hpet_available;
}

uint64_t hpet_counter(void)
{
    if(!hpet_available)
        return 0;

    return hpet_read(HPET_REG_MAIN_COUNTER);
}

uint64_t hpet_get_msec(void)
{
    if (!hpet_available)
        return 0;

    uint64_t c = hpet_counter();
    return c / hpet_ticks_per_ms;
}

uint64_t hpet_get_nsec(void)
{
    return hpet_get_msec() * 1000000ULL;
}

void hpet_sleep_msec(uint64_t ms)
{
    if (!hpet_available) {
        for (volatile uint64_t i = 0; i < ms * 100000; ++i) {
            __asm__ volatile("pause");
        }

        return;
    }

    uint64_t start = hpet_get_msec();
    uint64_t target = start + ms;

    while (hpet_get_msec() < target) {
        __asm__ volatile("pause");
    }
}
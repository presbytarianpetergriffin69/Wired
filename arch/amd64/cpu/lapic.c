#include <stdint.h>
#include <system.h>
#include <acpi.h>
#include <lapic.h>
#include <hpet.h>

static volatile uint32_t *lapic_mmio = NULL;
static uint32_t lapic_ticks_per_ms = 0;

static inline void lapic_write(uint32_t reg, uint32_t value)
{
    lapic_mmio[reg / 4] = value;
}

static inline uint32_t lapic_read(uint32_t reg)
{
    return lapic_mmio[reg / 4];
}

void lapic_init(uint64_t lapic_base_phys)
{
    if (!lapic_base_phys) {
        panic("[LAPIC] No LAPIC base provided\n");
        return;
    }

    lapic_mmio = (volatile uint32_t *)ACPI_PHYS_TO_VIRT(lapic_base_phys);

    uint32_t svr = lapic_read(LAPIC_REG_SVR);
    svr |= 0x100;
    svr = (svr & ~0xFFu) | 0xFFu;
    lapic_write(LAPIC_REG_SVR, svr);

    lapic_write(LAPIC_REG_LVT_TIMER, 1u << 16);

    kprintf("[LAPIC] Initialized at base=0x%016llx\n",
            (unsigned long long)lapic_base_phys);
}

void lapic_eoi(void)
{
    if (lapic_mmio)
        lapic_write(LAPIC_REG_EOI, 0);
}

void lapic_sleep_msec(uint64_t ms)
{
    if (lapic_ticks_per_ms == 0)
        return;

    uint64_t total_ticks = ms * (uint64_t)lapic_ticks_per_ms;

    while (total_ticks > 0) {
        uint32_t this_round;

        if (total_ticks > 0xFFFFFFFFull)
            this_round = 0xFFFFFFFFu;
        else
            this_round = (uint32_t)total_ticks;

        lapic_write(LAPIC_REG_DIVIDE, LAPIC_TIMER_DIVIDE_16);
        lapic_write(LAPIC_REG_LVT_TIMER,
                    LAPIC_TIMER_VECTOR |
                    LAPIC_TIMER_MODE_ONE_SHOT);

        lapic_write(LAPIC_REG_INIT_CNT, this_round);

        while (lapic_read(LAPIC_REG_CUR_CNT) != 0)
            __asm__ volatile("pause");

        total_ticks -= this_round;
    }
}

static void lapic_timer_calibrate(void)
{
    if (!lapic_mmio)
        return;

    if (hpet_get_freq_hz() == 0) {
        kprintf("[LAPIC] No HPET for calibration\n");
        return;
    }

    lapic_write(LAPIC_REG_DIVIDE, LAPIC_TIMER_DIVIDE_16);

    lapic_write(LAPIC_REG_LVT_TIMER,
                LAPIC_TIMER_VECTOR | LAPIC_TIMER_MODE_ONE_SHOT | (1u << 16));

    uint32_t start = 0xFFFFFFFFu;
    lapic_write(LAPIC_REG_INIT_CNT, start);

    hpet_sleep_msec(10);

    uint32_t cur = lapic_read(LAPIC_REG_CUR_CNT);
    uint32_t elapsed = start - cur;

    if (elapsed == 0) {
        kprintf("[LAPIC] calibration failed: elapsed=0\n");
        lapic_ticks_per_ms = 0;
        return;
    }

    lapic_ticks_per_ms = elapsed / 10;
    kprintf("[LAPIC] Calibration: %u ticks/ms (divide=16)\n",
            lapic_ticks_per_ms);

    lapic_write(LAPIC_REG_INIT_CNT, 0);
}

void lapic_timer_init(uint32_t hz)
{
    if (!lapic_mmio)
        return;

    if (lapic_ticks_per_ms == 0) {
        lapic_timer_calibrate();
        if (lapic_ticks_per_ms == 0) {
            lapic_ticks_per_ms = 1000000; // arbitrary guess
            kprintf("[LAPIC] No calibration; using default ticks/ms=%u\n",
                    lapic_ticks_per_ms);
        }
    }

    if (hz == 0)
        hz = 1000;

    uint32_t period_ms = 1000 / hz;
    if (period_ms == 0)
        period_ms = 1;

    uint32_t initial_count = lapic_ticks_per_ms * period_ms;

    kprintf("[LAPIC] timer: hz=%u, period=%ums, init_count=%u\n",
            hz, period_ms, initial_count);

    lapic_write(LAPIC_REG_DIVIDE, LAPIC_TIMER_DIVIDE_16);

    lapic_write(LAPIC_REG_LVT_TIMER,
                LAPIC_TIMER_VECTOR | LAPIC_TIMER_MODE_PERIODIC);

    lapic_write(LAPIC_REG_INIT_CNT, initial_count);

}

void lapic_timer_tick(void)
{
    static uint64_t ticks = 0;
    ticks++;
}
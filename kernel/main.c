#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <limine.h>
#include <console.h>
#include <font8x16.h>
#include <system.h>
#include <gdt.h>
#include <serial.h>
#include <io.h>
#include <tss.h>
#include <idt.h>
#include <acpi.h>
#include <hpet.h>
#include <lapic.h>
#include <pmm.h>
#include <page.h>

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST_ID,
    .revision = 0
};

void kmain(void)
{
    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        panic("No framebuffer from Limine");
    }

    struct limine_framebuffer *fb =
        framebuffer_request.response->framebuffers[0];

    uint32_t width  = fb->width;
    uint32_t height = fb->height;

    serial_init();

    console_init(fb);

    starfield_init();

    kprintf("======================\n");
    kprintf("= Welcome to WIREDOS =\n");
    kprintf("======================\n");

    kprintf("\n");

    gdt_init();
    tss_init();
    idt_init();
    
    acpi_init();

    acpi_selftest();

    uint64_t lapic_base = acpi_get_lapic_base();
    kprintf("[DEBUG] LAPIC base = 0x%016llx\n",
            (unsigned long long)lapic_base);

    hpet_init();
    lapic_init(lapic_base);
    lapic_timer_init(1000);

    mm_init();

    kprintf("mm works");

    __asm__ volatile ("int3");

    for (;;);
}
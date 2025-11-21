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

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

extern volatile struct limine_rsdp_request rsdp_request;
extern volatile struct limine_hhdm_request hhdm_request;
extern volatile struct limine_memmap_request memmap_request;

void kmain(void)
{

    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        halt();
    }

    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];

    serial_init();

    console_init(framebuffer);

    console_print("Welcome to the Wired\n");

    gdt_init();
    tss_init();
    idt_init();

    console_print("GDT initialized\n");

    console_print("acpi initialized\n");

    serial_puts("test\n");

    acpi_init();

    panic("If you get here everything works :-)");

    halt();
}
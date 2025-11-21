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

    serial_puts("test\n");

    acpi_init();

    console_print("acpi initialized\n");

    panic("If you get here everything works :-)");

    halt();
}
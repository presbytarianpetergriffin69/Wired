#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <serial.h>
#include <io.h>

static bool initialized = false;

#define KERNEL_COM_TTY 0x3F8

int serial_init(uint16_t base)
{
    uint16_t base = KERNEL_COM_TTY;

    outb(base + 1, 0x00);

    outb(base + 3, 0x80);
    outb(base + 0, 0x01);
    outb(base + 1, 0x00);

    outb(base + 3, 0x03);
    outb(base + 2, 0xC7);
    outb(base + 4, 0x0B);
}

void serial_putc(char c)
{
    if (!initialized)
        serial_init();

    outb(KERNEL_COM_TTY + 0, (uint8_t)c);
}

void serial_puts(const char *s)
{
    while (*s) {
        if (*s == '\n')
            serial_putc('\r');
        serial_putc(*s++);
    }
}
#include <system.h>
#include <console.h>
#include <stdarg.h>
#include <serial.h>

void kputc(char c)
{
    console_putc(c);
}

void kprint(const char *s) 
{
    while (*s) console_putc(*s++);
}

static void kprint_hex64(uint64_t n)
{
    static const char *hex = "0123456789ABCDEF";
    for (int i = 60; i >= 0; i -=4)
        console_putc(hex[(n >> i) & 0xF]);
}

void kprintf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    while (*fmt) {
        if (*fmt == '%') {
            fmt++;

            switch (*fmt) {
            case 'c':
                console_putc((char)va_arg(args, int));
                break;
            case 's':
                kprint(va_arg(args, const char*));
                break;
            case 'x':
            case 'p':
                kprint_hex64(va_arg(args, uint64_t));
                break;
            case '%':
                console_putc('%');
                break;
            default:
                console_putc('?');
            }
        } else {
            console_putc(*fmt);
        }

        fmt++;
    }

    va_end(args);
}

void klog(const char *s) {
    serial_puts(s);
}

NORETURN void halt(void) 
{
    for (;;) {
        asm volatile ("hlt");
    }
}

static void dump_regs(void) {
    uint64_t rsp, rbp;
    uint64_t cr0, cr2, cr3, cr4;
    void *rip = __builtin_return_address(0);

    asm volatile("mov %%rsp, %0" : "=r"(rsp));
    asm volatile("mov %%rbp, %0" : "=r"(rbp));
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    asm volatile("mov %%cr2, %0" : "=r"(cr2));
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    asm volatile("mov %%cr4, %0" : "=r"(cr4));

    kprint("\nRegisters:\n");
    kprintf("RIP=%p\n", rip);
    kprintf("RSP=%p  RBP=%p\n", rsp, rbp);
    kprintf("CR0=%p  CR2=%p\n", cr0, cr2);
    kprintf("CR3=%p  CR4=%p\n", cr3, cr4);
}

NORETURN void panic(const char *msg) 
{
    kprint("!!! STOP ERROR !!!\n");
    kprint(msg);
    kprint("\n");

    dump_regs();

    kprint("\nSystem execution halted\n");
    halt();
}
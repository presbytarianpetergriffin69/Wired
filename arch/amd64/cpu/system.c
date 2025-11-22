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

static void kprint_hex64(uint64_t v, int width, int uppercase)
{
    const char *digits = uppercase ? "0123456789ABCDEF"
                                   : "0123456789abcdef";
    char buf[32];
    int i = 0;

    do {
        buf[i++] = digits[v & 0xF];
        v >>= 4;
    } while (v && i < (int)sizeof(buf));

    while (i < width && i < (int)sizeof(buf))
        buf[i++] = '0';

    while (i--)
        console_putc(buf[i]);
}

static void kprint_dec_signed(long long v)
{
    char buf[32];
    int i = 0;
    int neg = 0;

    if (v < 0) {
        neg = 1;
        v = -v;
    }

    do {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    } while (v && i < (int)sizeof(buf));

    if (neg)
        buf[i++] = '-';

    while (i--) console_putc(buf[i]);
}

static void kprint_dec_unsigned(unsigned long long v)
{
    char buf[32];
    int i = 0;

    do {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    } while (v && i < (int)sizeof(buf));

    while (i--) console_putc(buf[i]);
}

void kprintf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    for (const char *p = fmt; *p; p++) {
        if (*p != '%') {
            console_putc(*p);
            continue;
        }

        p++;

        int zero_pad = 0;
        int width    = 0;
        int long_count = 0;

        if (*p == '0') {
            zero_pad = 1;
            p++;
        }

        while (*p >= '0' && *p <= '9') {
            width = width * 10 + (*p - '0');
            p++;
        }

        while (*p == 'l') {
            long_count++;
            p++;
        }

        switch (*p) {
        case 'c': {
            int c = va_arg(args, int);
            console_putc((char)c);
            break;
        }
        case 's': {
            const char *s = va_arg(args, const char *);
            if (!s) s = "(null)";
            while (*s)
                console_putc(*s++);
            break;
        }
        case 'd':
        case 'i': {
            long long v;
            if (long_count >= 2)
                v = va_arg(args, long long);
            else if (long_count == 1)
                v = va_arg(args, long);
            else
                v = va_arg(args, int);
            kprint_dec_signed(v);
            break;
        }
        case 'u': {
            unsigned long long v;
            if (long_count >= 2)
                v = va_arg(args, unsigned long long);
            else if (long_count == 1)
                v = va_arg(args, unsigned long);
            else
                v = va_arg(args, unsigned int);
            kprint_dec_unsigned(v);
            break;
        }
        case 'x':
        case 'X': {
            unsigned long long v;
            if (long_count >= 2)
                v = va_arg(args, unsigned long long);
            else if (long_count == 1)
                v = va_arg(args, unsigned long);
            else
                v = va_arg(args, unsigned int);

            if (width == 0) width = 1;
            kprint_hex64((uint64_t)v, width, *p == 'X');
            break;
        }
        case 'p': {
            void *ptr = va_arg(args, void *);
            uint64_t v = (uint64_t)(uintptr_t)ptr;
            kprint("0x");
            kprint_hex64(v, 16, 0);
            break;
        }
        case '%':
            console_putc('%');
            break;

        default:
            // unknown specifier: print literally
            console_putc('%');
            console_putc(*p);
            break;
        }
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
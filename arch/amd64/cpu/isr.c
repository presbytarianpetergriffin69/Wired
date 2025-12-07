#include <stdint.h>
#include <system.h>
#include <isr.h>
#include <lapic.h>

typedef struct
{
    irq_handler_t handler;
    void            *ctx;
} irq_entry_t;

static irq_entry_t irq_table[256]; // 0 - 255

static const char *exception_names[32] =
{
    "Division Error",
    "Debug",
    "Non-maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound range exceeded",
    "Invalid opcode",
    "Device not available",
    "Double fault",
    "Coprocesor segment overrun",
    "Invalid tss",
    "Segment not present",
    "Stack segment fault",
    "General protection error",
    "Page fault",
    "Reserved",
    "x87 Floating point exception",
    "Alignment check",
    "Machine check",
    "SIMD floating point exception",
    "Virtualization exception",
    "Control protecton exception",
    "Reserved",
    "Hypervisor injection exception",
    "VMM communication exception",
    "Security error",
    "Reserved",
    "Triple fault",
};

static void dump_common_frame(isr_regs_t *r)
{
    kprintf("\n[EXC] RIP=%p  CS=%p  RFLAGS=%p\n",
            r->rip, r->cs, r->rflags);
    kprintf("[EXC] RSP=%p  SS=%p\n", r->rsp, r->ss);

    kprintf("[EXC] RAX=%p RBX=%p RCX=%p RDX=%p\n",
            r->rax, r->rbx, r->rcx, r->rdx);
    kprintf("[EXC] RSI=%p RDI=%p RBP=%p\n",
            r->rsi, r->rdi, r->rbp);
    kprintf("[EXC] R8 =%p R9 =%p R10=%p R11=%p\n",
            r->r8, r->r9, r->r10, r->r11);
    kprintf("[EXC] R12=%p R13=%p R14=%p R15=%p\n",
            r->r12, r->r13, r->r14, r->r15);
}

static void handle_page_fault(isr_regs_t *r)
{
    uint64_t cr2;
    asm volatile("mov %%cr2, %0" : "=r"(cr2));

    uint64_t ec = r->err_code;

    int present = (ec & 0x1) != 0;
    int write = (ec & 0x2) != 0;
    int user = (ec & 0x4) != 0;
    int reserved = (ec & 0x8) != 0;
    int ifetch = (ec & 0x10) != 0;

    kprintf("[PF] Page fault at address %p\n", cr2);
    kprintf("[PF] Error code = %p\n", ec);
    kprintf("[PF] present=%u write=%u user=%u reserved=%u ifetch=%u\n",
            present, write, user, reserved, ifetch);

    dump_common_frame(r);

    panic("Page fault");
}

void irq_register(uint8_t vec, irq_handler_t handler, void *ctx)
{
    irq_table[vec].handler = handler;
    irq_table[vec].ctx     = ctx;
}

void irq_unregister(uint8_t vec)
{
    irq_table[vec].handler = NULL;
    irq_table[vec].ctx     = NULL;
}

void isr_dispatch(isr_regs_t *r)
{
    uint32_t vec = r->int_no;

    // --- CPU exceptions ---
    if (vec < 32) {
        const char *name = exception_names[vec];

        kprintf("\n=== CPU EXCEPTION %u: %s ===\n",
                (unsigned)vec, name);
        kprintf("[EXC] error code %p\n", (void *)r->err_code);

        if (vec == 14) {
            handle_page_fault(r);
        } else {
            dump_common_frame(r);
            panic("Unhandled CPU Exception");
        }

        return;
    }

    if (vec == LAPIC_TIMER_VECTOR) {

        lapic_timer_tick();  
        lapic_eoi();         
        return;
    }

    irq_entry_t *entry = &irq_table[vec];

    if (entry->handler) {
        entry->handler(r, entry->ctx);
    } else {
        panic("Unhandled IRQ vector %u\n");
    }

    kprintf("[INT] Interrupt %u received\n");

    lapic_eoi();

}
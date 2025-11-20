[BITS 64]

section .text

global gdt_load
gdt_load:
    lgdt [rdi]

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    push 0x08
    lea rax, [rel .gdt_done]
    push rax
    retfq

.gdt_done:
    ret

global tss_load
tss_load:
    mov ax, di
    ltr ax
    ret

global idt_load
idt_load:
    lidt [rdi]
    ret

global isr_default_stub
isr_default_stub:
    cli

.hang:
    hlt
    jmp .hang
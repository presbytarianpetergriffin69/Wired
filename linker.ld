ENTRY(_start)

KERNEL_VMA = 0xffffffff80000000;

SECTIONS
{
  /* Put the kernel at virtual KERNEL_VMA + 2 MiB.
   * Limine will load it at the corresponding physical address.
   */
  . = KERNEL_VMA + 0x200000;

  /* Code */
  .text ALIGN(0x1000) : AT(ADDR(.text) - KERNEL_VMA)
  {
    *(.text .text.*)
  }

  /* Read-only data + Limine requests */
  .rodata ALIGN(0x1000) : AT(ADDR(.rodata) - KERNEL_VMA)
  {
    *(.rodata .rodata.*)

    KEEP(*(.limine_requests_start))
    KEEP(*(.limine_requests))
    KEEP(*(.limine_requests_end))
  }

  /* Writable data */
  .data ALIGN(0x1000) : AT(ADDR(.data) - KERNEL_VMA)
  {
    *(.data .data.*)
  }

  /* Zero-initialized data */
  .bss ALIGN(0x1000) : AT(ADDR(.bss) - KERNEL_VMA)
  {
    *(COMMON)
    *(.bss .bss.*)
  }
}

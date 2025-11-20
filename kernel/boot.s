.section .text
.global _start
.code64
_start:
				call kmain
.hang:
				hlt
				jmp .hang

#pragma once
#include <stddef.h>
#include <stdint.h>
#include <limine.h>

void console_init(struct limine_framebuffer *fb);
void console_clear(void);
void console_putc(char c);
void console_print(const char *s);
void console_fill(uint32_t color);
void starfield_init(void);
void starfield_twinkle(void);
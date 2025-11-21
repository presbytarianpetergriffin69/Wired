#pragma once

#include <stdint.h>
#include <stddef.h>

void serial_init(uint16_t base);
void serial_putc(char c);
void serial_puts(const char *s);
#pragma once

#include <stdint.h>
#include <stddef.h>

void serial_init(void);
void serial_putc(char c);
void serial_write(const char *s);
void serial_write_len(const char *s, size_t len);
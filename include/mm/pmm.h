/**
 * pmm.h
 * B Iohannes
 * 
 * Physical memory manager responsible for 
 * tracking frames and setting up for
 * page allocation
 * Copyright (c) 2025
 *
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <system.h>

#ifndef PMM_PAGE_SIZE
#define PMM_PAGE_SIZE 0x1000UL
#endif

typedef uint64_t paddr_t;

enum
{
    PMM_ALLOC_NONE = 0x0,
    PMM_ALLOC_HIGH = 0x1,
    PMM_ALLOC_LOW  = 0x2,
};

void     pmm_init(uint64_t mem_size_bytes);
void     pmm_mark_region_free(uintptr_t base, size_t size);
void     pmm_mark_region_used(uintptr_t base, size_t size);
uintptr_t pmm_alloc_frame(void);
void     pmm_free_frame(uintptr_t paddr);
uintptr_t pmm_alloc_pages(size_t count);
void     pmm_free_pages(uintptr_t base, size_t count);
uint64_t pmm_total_memory(void);
uint64_t pmm_used_memory(void);
uint64_t pmm_free_memory(void);
void     mm_init(void);
void     *malloc(size_t size);
void     free(void *ptr);
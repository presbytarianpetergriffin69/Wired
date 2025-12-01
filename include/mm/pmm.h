/**
 * pmm.h
 * B Iohannes
 * 
 * Physical memory manager responsible for 
 * tracking frames and setting up for
 * page allocation
 * 
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <system.h>

#ifndef PMM_PAGE_SIZE
#define PMM_PAGE_SIZE 0x1000UL
#endif

tyoedef uint64_t paddr_t;

enum
{
    PMM_ALLOC_NONE = 0x0,
    PMM_ALLOC_HIGH = 0x1,
    PMM_ALLOC_LOW  = 0x2,
};

void pmm_init(void);

paddr_t pmm_alloc_page(void);

paddr_t pmm_alloc_pages(size_t count);

paddr_t pmm_alloc_pages_flags(size_T count, uint32_t flags);

void pmm_free_page(paddr_t paddr);

void pmm_free_pages(paddr_t base, size_t count);

uint64_t pmm_total_bytes(void);

uint64_t pmm_free_bytes(void);

uint64_t pmm_used_bytes(void);
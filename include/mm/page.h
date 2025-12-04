/**
 * page.h
 * B Iohannes
 *  
 * 
 * Copyright (c) 2025
 * 
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <pmm.h>
#include <system.h>

#define PAGE_SIZE       PMM_PAGE_SIZE
#define PAGE_SHIFT      12
#define PAGE_MASK       (~(PAGE_SIZE - 1ULL))

#define PAGE_TABLE_ENTRIES 512

#define PML4_INDEX(va)  (((uint64_t)(va) >> 39) & 0x1FFULL)
#define PDP_INDEX(va)   (((uint64_t)(va) >> 30) & 0x1FFULL)
#define PD_INDEX(va)    (((uint64_t)(va) >> 21) & 0x1FFULL)
#define PT_INDEX(va)    (((uint64_t)(va) >> 12) & 0x1FFULL)

#define PTE_FRAME(e)    ((paddr_t)((e) & 0x000FFFFFFFFFF000ULL))

typedef uint64_t pte_t;

enum {
    PTE_PRESENT         = 1ULL << 0,
    PTE_WRITABLE        = 1ULL << 1,
    PTE_USER            = 1ULL << 2,
    PTE_PWT             = 1ULL << 3,
    PTE_PCD             = 1ULL << 4,
    PTE_ACCESSED        = 1ULL << 5,
    PTE_DIRTY           = 1ULL << 6,
    PTE_HUGE            = 1ULL << 7,
    PTE_GLOBAL          = 1ULL << 8,
    PTE_NX              = 1ULL << 63
};

#define PTE_KERNEL_RW   (PTE_PRESENT | PTE_WRITABLE)
#define PTE_KERNEL_RX   (PTE_PRESENT)
#define PTE_USER_RW     (PTE_PRESENT | PTE_WRITABLE | PTE_USER)
#define PTE_USER_RX     (PTE_PRESENT | PTE_USER) 

// page table types

typedef struct page_table
{
    pte_t entries[PAGE_TABLE_ENTRIES];
} ALIGNED(PAGE_SIZE) page_table_t;

typedef struct page_map
{
    paddr_t pml4_phys;
    page_table_t *pml4_virt;
} page_map_t;

// initialize functions

void page_init(void);

page_map_t *paging_get_kernel_map(void);

// address space functions

page_map_t *paging_new_addr_space(void);

void paging_destroy_addr_space(page_map_t *map);

void paging_activate(page_map_t *map);

paddr_t paging_current_cr3(void);

// mapping / unmapping

int paging_map_page(page_map_t *map,
                    uintptr_t virt,
                    paddr_t phys,
                    uint64_t flags);

int paging_unmap_page(page_map_t *map,
                    uintptr_t virt,
                    int free_phys);

paddr_t paging_virt_to_phys(page_map_t *map, uintptr_t virt);

int paging_ensure_tables(page_map_t *map,
                         uintptr_t virt,
                         size_t length,
                         uint64_t flags);

void paging_invlpg(uintptr_t virt);

void paging_flush_tlb(void);
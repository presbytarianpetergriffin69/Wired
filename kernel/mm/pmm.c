/**
 * pmm.c
 * B Iohannes
 * 
 * Physical memory manager using Limine memmap
 * With Kernel level heap
 * and 4 level paging
 * Copyright (c) 2025
 * 
 */


#include <stdint.h>
#include <stddef.h>

#include <pmm.h>
#include <page.h>
#include <system.h>
#include <limine.h>

// limine requests from main.c

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0
};

#ifndef PMM_MAX_PHYS_MEM
#define PMM_MAX_PHYS_MEM (4ULL * 1024ULL * 1024ULL * 1024ULL)
#endif

#define PMM_MAX_FRAMES   (PMM_MAX_PHYS_MEM / PMM_PAGE_SIZE)
#define PMM_BITMAP_SIZE  (PMM_MAX_FRAMES / 8)

static uint8_t pmm_bitmap[PMM_BITMAP_SIZE];
static uint64_t pmm_frames_total = 0;
static uint64_t pmm_frames_used = 0;
static uint64_t kernel_hhdm_offset = 0;

// static helpers

static inline void pmm_set_frame(size_t frame)
{
    size_t byte = frame / 8;
    uint8_t bit = (uint8_t)(frame % 8);
    pmm_bitmap[byte] |= (uint8_t)(1U << bit);
}

static inline void pmm_clear_frame(size_t frame)
{
    size_t byte = frame / 8;
    uint8_t bit = (uint8_t)(frame % 8);
    pmm_bitmap[byte] &= (uint8_t)~(1U << bit);
}

static inline int pmm_test_frame(size_t frame)
{
    size_t byte = frame / 8;
    uint8_t bit = (uint8_t)(frame % 8);
    return (pmm_bitmap[byte] & (uint8_t)(1U << bit)) != 0;
}

// core functions

void pmm_init(uint64_t mem_size_bytes)
{
    if (mem_size_bytes > PMM_MAX_PHYS_MEM) {
        mem_size_bytes = PMM_MAX_PHYS_MEM;
    }

    pmm_frames_total = mem_size_bytes / PMM_PAGE_SIZE;
    if (pmm_frames_total > PMM_MAX_FRAMES) {
        pmm_frames_total = PMM_MAX_FRAMES;
    }

    memset(pmm_bitmap, 0xFF, sizeof(pmm_bitmap));
    pmm_frames_used = pmm_frames_total;
}

void pmm_mark_region_free(uintptr_t base, size_t size)
{
    if (size == 0 || pmm_frames_total == 0) {
        return;
    }

    uint64_t start_frame = base / PMM_PAGE_SIZE;
    uint64_t end_frame = (base + size - 1) / PMM_PAGE_SIZE;

    if (start_frame >= pmm_frames_total) {
        return;
    }

    if (end_frame >= pmm_frames_total) {
        end_frame = pmm_frames_total - 1;
    }

    for (uint64_t f = start_frame; f <= end_frame; ++f) {
        if (f == 0) {
            continue; // never free frame 0
        }
        if (pmm_test_frame((size_t)f)) {
            pmm_clear_frame((size_t)f);
            if (pmm_frames_used > 0) {
                --pmm_frames_used;
            }
        }
    }
}

static int64_t pmm_find_first_free_frame(void)
{
    if (pmm_frames_total == 0) {
        return -1;
    }

    size_t bitmap_bytes = (size_t)((pmm_frames_total + 7) / 8);

    for (size_t byte = 0; byte < bitmap_bytes; ++byte) {
        if (pmm_bitmap[byte] == 0xFF) {
            continue;
        }
        for (uint8_t bit = 0; bit < 8; ++bit) {
            size_t frame = byte * 8 + bit;
            if (frame >= pmm_frames_total) {
                break;
            }
            if (!pmm_test_frame(frame)) {
                return (int64_t)frame;
            }
        }
    }

    return -1;
}

uintptr_t pmm_alloc_frame(void)
{
    int64_t frame = pmm_find_first_free_frame();
    if (frame < 0) {
        return 0;
    }

    pmm_set_frame((size_t)frame);
    ++pmm_frames_used;

    return (uintptr_t)frame * PMM_PAGE_SIZE;
}

void pmm_free_frame(uintptr_t paddr)
{
    if (paddr == 0) {
        return;
    }

    if ((paddr & (PMM_PAGE_SIZE -1 )) != 0) {
        return;
    }

    uint64_t frame = paddr / PMM_PAGE_SIZE;
    if (frame >= pmm_frames_total) {
        return;
    }

    if (pmm_test_frame((size_t)frame)) {
        pmm_clear_frame((size_t)frame);
        if (pmm_frames_used > 0) {
            --pmm_frames_used;
        }
    }
}

static int64_t pmm_find_free_run(size_t count)
{
    if (count == 0 || pmm_frames_total == 0) {
        return -1;
    }

    size_t free_run = 0;
    size_t start    = 0;

    for (size_t frame = 0; frame < pmm_frames_total; ++frame) {
        if (!pmm_test_frame(frame)) {
            if (free_run == 0) {
                start = frame;
            }
            ++free_run;

            if (free_run == count) {
                return (int64_t)start;
            }
        } else {
            free_run = 0;
        }
    }

    return -1;
}

uintptr_t pmm_alloc_pages (size_t count)
{
    if (count == 0) {
        return 0;
    }

    int64_t start = pmm_find_free_run(count);
    if (start < 0) {
        return 0;
    }

    for (size_t i = 0; i < count; ++i) {
        pmm_set_frame((size_t)start + i);
    }

    pmm_frames_used += count;

    return (uintptr_t)start * PMM_PAGE_SIZE;
} 

void pmm_free_pages(uintptr_t base, size_t count)
{
    if (count == 0) {
        return;
    }

    if ((base & (PMM_PAGE_SIZE - 1)) != 0) {
        return;
    }

    uint64_t start_frame = base / PMM_PAGE_SIZE;
    if (start_frame >= pmm_frames_total) {
        return;
    }

    uint64_t end_frame = start_frame + count - 1;
    if (end_frame >= pmm_frames_total) {
        end_frame = pmm_frames_total - 1;
    }

    for (uint64_t f = start_frame; f <= end_frame; ++f) {
        if (pmm_test_frame((size_t)f)) {
            pmm_clear_frame((size_t)f);
            if (pmm_frames_used > 0) {
                --pmm_frames_used;
            }
        }
    }
}

uint64_t pmm_total_memory(void)
{
    return pmm_frames_total * PMM_PAGE_SIZE;
}

uint64_t pmm_used_memory(void)
{
    return pmm_frames_used * PMM_PAGE_SIZE;
}

uint64_t pmm_free_memory(void)
{
    return pmm_total_memory() - pmm_used_memory();
}

// limine memmap pmm init

static inline uint64_t round_up_page(uint64_t x)
{
    return (x + (PMM_PAGE_SIZE -1)) & ~(PMM_PAGE_SIZE - 1);
}

void pmm_init_limine(void)
{
    const struct limine_memmap_response *memmap = memmap_request.response;

    if (!memmap || memmap->entry_count == 0) {
        panic("[PMM] No limine memory map\n");
    }

    if (!hhdm_request.response) {
        panic("No limine hhdm response\n");
    }
    kernel_hhdm_offset = hhdm_request.response->offset;

    // find highest physical addr

    uint64_t max_phys = 0;
    for (uint64_t i = 0; i < memmap->entry_count; ++i) {
        const struct limine_memmap_entry *e = memmap->entries[i];
        uint64_t end = e->base + e->length;
        if (end > max_phys) {
            max_phys = end;
        }
    }
    max_phys = round_up_page(max_phys);

    // init pmm

    pmm_init (max_phys);

    // mark usable space as free

    for (uint64_t i = 0; i < memmap->entry_count; ++i) {
        const struct limine_memmap_entry *e = memmap->entries[i];

        if (e->type == LIMINE_MEMMAP_USABLE) {
            uint64_t base   = e->base;
            uint64_t length = e->length;

            uint64_t aligned_base = round_up_page(base);
            uint64_t end          = base + length;
            uint64_t aligned_end  = end & ~(PMM_PAGE_SIZE - 1);

            if (aligned_end > aligned_base) {
                uint64_t aligned_len = aligned_end - aligned_base;
                pmm_mark_region_free((uintptr_t)aligned_base,
                                     (size_t)aligned_len);
            }
        }
    }
}

// kernel level heap

typedef struct heap_block 
{
    size_t size;
    struct heap_block *next;
    int free;
} heap_block_t;

static heap_block_t *heap_head = NULL;
static uint8_t      *heap_base = NULL;
static size_t       heap_size  = 0;

static void heap_init(void)
{
    if (heap_head) {
        return;
    }

    size_t heap_pages = 256;
    uintptr_t heap_phys = pmm_alloc_pages(heap_pages);
    if (!heap_phys) {
        panic("[PMM] kernel heap alloc failed");
    }

    heap_base = (uint8_t *)(heap_phys + kernel_hhdm_offset);
    heap_size = heap_pages * PMM_PAGE_SIZE;

    heap_head = (heap_block_t *)heap_base;
    heap_head->size = heap_size - sizeof(heap_block_t);
    heap_head->next = NULL;
    heap_head->free = 1;
}

void *malloc(size_t size)
{
    if (size == 0) {
        return NULL;
    }

    if (!heap_head) {
        heap_init();
    }
    
    const size_t align = 16;
    size = (size + align - 1) & ~(align - 1);

    heap_block_t *curr = heap_head;

    while (curr) {
        if (curr->free && curr->size >= size) {
            size_t remain = curr->size - size;

            if (remain > sizeof(heap_block_t) + 16) {
                // Split block
                uint8_t *new_addr = (uint8_t *)curr + sizeof(heap_block_t) + size;
                heap_block_t *newblk = (heap_block_t *)new_addr;

                newblk->size = remain - sizeof(heap_block_t);
                newblk->free = 1;
                newblk->next = curr->next;

                curr->size = size;
                curr->next = newblk;
            }

            curr->free = 0;
            return (uint8_t *)curr + sizeof(heap_block_t);
        }
        curr = curr->next;
    }

    return NULL;
}

void free(void *ptr)
{
    if (!ptr) {
        return;
    }

    heap_block_t *blk = (heap_block_t *)((uint8_t *)ptr - sizeof(heap_block_t));
    blk->free = 1;

    // Coalesce adjacent free blocks
    heap_block_t *curr = heap_head;
    while (curr) {
        if (curr->free) {
            while (curr->next && curr->next->free) {
                curr->size += sizeof(heap_block_t) + curr->next->size;
                curr->next  = curr->next->next;
            }
        }
        curr = curr->next;
    }
}

// paging
// 4 level

#define PTE_ADDR_MASK  0x000FFFFFFFFFF000ULL

static page_map_t kernel_map;

static page_table_t *alloc_page_table(paddr_t *out_phys)
{
    uintptr_t phys = pmm_alloc_frame();
    if (!phys) {
        panic("[PAGE] alloc_page_table: Out of memory!");
    }

    *out_phys = (paddr_t)phys;
    page_table_t *virt = (page_table_t *)(phys + kernel_hhdm_offset);
    memset(virt, 0, sizeof(page_table_t));
    return virt;
}

static pte_t *get_pte (page_map_t *map, uintptr_t virt, int create)
{
    page_table_t *pml4 = map->pml4_virt;

    uint64_t pml4_i = PML4_INDEX(virt);
    uint64_t pdp_i  = PDP_INDEX(virt);
    uint64_t pd_i   = PD_INDEX(virt);
    uint64_t pt_i   = PT_INDEX(virt);

    pte_t *pml4e = &pml4->entries[pml4_i];
    page_table_t *pdp;

    if (!(*pml4e & PTE_PRESENT)) {
        if (!create) return NULL;
        paddr_t pdp_phys;
        pdp = alloc_page_table(&pdp_phys);
        *pml4e = (pdp_phys & PTE_ADDR_MASK) | PTE_KERNEL_RW;
    } else {
        paddr_t pdp_phys = PTE_FRAME(*pml4e);
        pdp = (page_table_t *)(pdp_phys + kernel_hhdm_offset);
    }

    pte_t *pdpe = &pdp->entries[pdp_i];
    page_table_t *pd;

    if (!(*pdpe & PTE_PRESENT)) {
        if (!create) return NULL;
        paddr_t pd_phys;
        pd = alloc_page_table(&pd_phys);
        *pdpe = (pd_phys & PTE_ADDR_MASK) | PTE_KERNEL_RW;
    } else {
        paddr_t pd_phys = PTE_FRAME(*pdpe);
        pd = (page_table_t *)(pd_phys + kernel_hhdm_offset);
    }

    pte_t *pde = &pd->entries[pd_i];
    page_table_t *pt;

    if (!(*pde & PTE_PRESENT)) {
        if (!create) return NULL;
        paddr_t pt_phys;
        pt = alloc_page_table(&pt_phys);
        *pde = (pt_phys & PTE_ADDR_MASK) | PTE_KERNEL_RW;
    } else {
        paddr_t pt_phys = PTE_FRAME(*pde);
        pt = (page_table_t *)(pt_phys + kernel_hhdm_offset);
    }

    return &pt->entries[pt_i];
}

void page_init(void)
{
    if(!memmap_request.response || !hhdm_request.response) {
        panic("[PAGE] page init called before limine init");
    }

    kernel_hhdm_offset = hhdm_request.response->offset;

    paddr_t pml4_phys = paging_current_cr3();
    page_table_t *pml4_virt = (page_table_t *)(pml4_phys + kernel_hhdm_offset);

    kernel_map.pml4_phys = pml4_phys;
    kernel_map.pml4_virt = pml4_virt;

    uint64_t max_phys = pmm_total_memory();

    // for (uint64_t phys = 0; phys < max_phys; phys += PAGE_SIZE) {
        // identity
    //    paging_map_page(&kernel_map, (uintptr_t)phys, (paddr_t)phys, PTE_KERNEL_RW);
        // hhdm
    //    paging_map_page(&kernel_map,
    //                    (uintptr_t)(phys + kernel_hhdm_offset),
    //                    (paddr_t)phys,
    //                    PTE_KERNEL_RW);
    //}

    //paging_activate(&kernel_map);
}

page_map_t *paging_get_kernel_map(void)
{
    return &kernel_map;
}

page_map_t *paging_new_addr_space(void)
{
    page_map_t *map = (page_map_t *)malloc(sizeof(page_map_t));
    if (!map) return NULL;

    paddr_t pml4_phys;
    page_table_t *pml4 = alloc_page_table(&pml4_phys);

    map->pml4_phys = pml4_phys;
    map->pml4_virt = pml4;

    return map;
}

void paging_destroy_addr_space(page_map_t *map)
{
    if (!map) return;
    free(map);
}

void paging_activate(page_map_t *map)
{
    if (!map) return;
    uint64_t cr3 = (uint64_t)map->pml4_phys;
    __asm__ volatile ("mov %0, %%cr3" :: "r"(cr3) : "memory");
}

paddr_t paging_current_cr3(void)
{
    uint64_t cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
    return (paddr_t)cr3;
}


int paging_map_page(page_map_t *map,
                    uintptr_t virt,
                    paddr_t phys,
                    uint64_t flags)
{
    pte_t *pte = get_pte(map, virt, 1);
    if (!pte) {
        return -1;
    }

    *pte = ((uint64_t)phys & PTE_ADDR_MASK) | flags;
    paging_invlpg(virt);
    return 0;
}

int paging_unmap_page(page_map_t *map,
                      uintptr_t virt,
                      int free_phys)
{
    pte_t *pte = get_pte(map, virt, 0);
    if (!pte || !(*pte & PTE_PRESENT)) {
        return -1;
    }

    if (free_phys) {
        paddr_t phys = PTE_FRAME(*pte);
        pmm_free_frame((uintptr_t)phys);
    }

    *pte = 0;
    paging_invlpg(virt);
    return 0;
}

paddr_t paging_virt_to_phys(page_map_t *map, uintptr_t virt)
{
    pte_t *pte = get_pte(map, virt, 0);
    if (!pte || !(*pte & PTE_PRESENT)) {
        return 0;
    }
    return PTE_FRAME(*pte) | (virt & (PAGE_SIZE - 1));
}

int paging_ensure_tables(page_map_t *map,
                         uintptr_t virt,
                         size_t length,
                         uint64_t flags)
{
    (void)flags;
    uintptr_t end = virt + length;
    for (uintptr_t addr = virt; addr < end; addr += PAGE_SIZE) {
        pte_t *pte = get_pte(map, addr, 1);
        if (!pte) return -1;
    }
    return 0;
}

void paging_invlpg(uintptr_t virt)
{
    __asm__ volatile ("invlpg (%0)" :: "r"(virt) : "memory");
}

void paging_flush_tlb(void)
{
    uint64_t cr3 = paging_current_cr3();
    __asm__ volatile ("mov %0, %%cr3" :: "r"(cr3) : "memory");
}

static const char *memmap_type_str(uint64_t type)
{
    switch (type) {
        case LIMINE_MEMMAP_USABLE:                return "USABLE";
        case LIMINE_MEMMAP_RESERVED:              return "RESERVED";
        case LIMINE_MEMMAP_ACPI_RECLAIMABLE:      return "ACPI_RECLAIM";
        case LIMINE_MEMMAP_ACPI_NVS:              return "ACPI_NVS";
        case LIMINE_MEMMAP_BAD_MEMORY:            return "BAD";
        case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:return "BOOTLOADER";
        case LIMINE_MEMMAP_FRAMEBUFFER:           return "FRAMEBUFFER";
        default:                                  return "UNKNOWN";
    }
} 

void pmm_debug_dump(void)
{
    const struct limine_memmap_response *memmap = memmap_request.response;

    if (!memmap) {
        kprintf("[PMM] memmap_response is null\n");
        return;
    }

    for (uint64_t i = 0; i < memmap->entry_count; ++i) {
        const struct limine_memmap_entry *e = memmap->entries[i];
        kprintf("[PMM] [%2llu] %s base=%016llx len=%016llx\n",
                (unsigned long long)i,
                memmap_type_str(e->type),
                (unsigned long long)e->base,
                (unsigned long long)e->length);
    }

    kprintf("[PMM] ===== stats =====\n");
    kprintf("[PMM] total: %llu KiB\n", (unsigned long long)(pmm_total_memory() / 1024));
    kprintf("[PMM] used : %llu KiB\n", (unsigned long long)(pmm_used_memory()  / 1024));
    kprintf("[PMM] free : %llu KiB\n", (unsigned long long)(pmm_free_memory()  / 1024));
    kprintf("[PMM] frames_total=%llu frames_used=%llu\n",
            (unsigned long long)pmm_frames_total,
            (unsigned long long)pmm_frames_used);

    kprintf("[PMM] HHDM offset: %016llx\n\n",
            (unsigned long long)kernel_hhdm_offset);
}

void mm_init(void)
{
    pmm_init_limine();
    page_init();

    pmm_debug_dump();
}
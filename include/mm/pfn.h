#pragma once 

#include <types.h>

typedef enum
{
    PFN_STATE_FREE,
    PFN_STATE_ZERO,
    PFN_STATE_STANDBY,
    PFN_STATE_MODIFIED,
    PFN_STATE_BAD,
    PFN_STATE_ACTIVE
} pfn_state_t;

typedef struct vm_pfn_entry
{
    paddr_t paddr;
    pfn_t index;
    pfn_state_t state;

    struct vm_region *region;
    vaddr_t vaddr;

    struct vm_pfn_entry *next;
    struct vm_pfn_entry *prev;

    uint64_t backing_offset;
    uint32_t refcount;
    uint32_t flags;
} vm_pfn_entry_t;

typedef struct vm_pfn_list
{
    vm_pfn_entry_t *head;
    vm_pfn_entry_t *tail;
    size_t count;
} vm_pfn_list_t;


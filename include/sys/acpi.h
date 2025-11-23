#pragma once

#include <stdint.h>
#include <system.h>

#ifndef ACPI_PHYS_TO_VIRT
#   define ACPI_PHYS_TO_VIRT(p) ((void *)(uintptr_t)(p))
#endif

/**
 * simple implementation for now,
 * will likely need revision down the line
 */

typedef struct XSDP_t
{
    char Signature[8];
    uint8_t Checksum;
    char OEMID[6];
    uint8_t Revision;
    uint32_t RsdtAddress;

    // acpi 2.0 fields
    uint32_t Length;
    uint64_t XsdtAddress;
    uint8_t ExtendedChecksum;
    uint8_t reserved[3];
} PACKED XSDP_t;

typedef struct ACPISDTHeader
{
    char Signature[4];
    uint32_t Length;
    uint8_t Revision;
    uint8_t Checksum;
    char OEMID[6];
    char OEMTableID[8];
    uint32_t OEMRevision;
    uint32_t CreatorID;
    uint32_t CreatorRevision;
} PACKED ACPISDTHeader;

struct acpi_hpet
{
    struct ACPISDTHeader header;
    uint8_t hardware_rev_id;
    uint8_t comparator_count : 5;
    uint8_t counter_size : 1;
    uint8_t reserved : 1;
    uint8_t legacy_replacement : 1;
    
    uint16_t vendorid;
    
    struct {
        uint8_t address_space_id;
        uint8_t register_bit_width;
        uint8_t register_bit_offset;
        uint8_t reserved;
        uint64_t address;
    } PACKED base_address;

    uint8_t hpet_number;
    uint16_t minimum_tick;
    uint8_t page_protection;
} PACKED;

extern ACPISDTHeader *g_xsdt;
extern ACPISDTHeader *g_rsdt;
extern XSDP_t *g_xsdp;

void acpi_init(void);
ACPISDTHeader *acpi_find_table(const char sig[4]);
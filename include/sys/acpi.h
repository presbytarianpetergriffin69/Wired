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

struct XSDP_t
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
} PACKED;

struct ACPISDTHeader
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
} PACKED;

struct XSDT
{
    struct ACPISDTHeader h;
    uint64_t entries[];
} PACKED;

struct RSDT
{
    struct ACPISDTHeader h;
    uint32_t entries[];
} PACKED;

struct MADT
{
    struct ACPISDTHeader h;
    uint32_t LocalApicAddress;
    uint32_t Flags;
    uint8_t entries[];
} PACKED;

void acpi_init(void);

// lookup by 4 letter sig
struct ACPISDTHeader *acpi_find_table(const char sig[4]);
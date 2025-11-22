#pragma once

#include <stdint.h>
#include <system.h>

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

 extern ACPISDTHeader *g_xsdt;
 extern ACPISDTHeader *g_rsdt;
 extern XSDP_t *g_xsdp;

 void acpi_init(void);
 ACPISDTHeader *acpi_find_table(const char sig[4]);
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

 extern XSDP_t *g_xsdp;

 void acpi_init(void);
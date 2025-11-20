#pragma once

#include <stdint.h>
#include <system.h>

struct acpi_rdsp
{
    char signature[4];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;

    // acpi 2.0 parameters
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} PACKED;

struct acpi_sdt_header
{
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} PACKED;

struct acpi_madt
{
    struct acpi_sdt_header header;
    uint32_t lapic_addr;
    uint32_t flags;
} PACKED;

struct acpi_madt_entry_header
{
    uint8_t type;
    uint8_t length;
} PACKED;

enum 
{
    MADT_LAPIC = 0,
    MADT_IOAPIC = 1,
    MADT_INT_OVERRIDE = 2,
    MADT_NMI_SRC = 3,
    MADT_LAPIC_NMI = 4,
    NADT_LAPIC_OVERRIDE = 5
};

struct madt_lapic
{
    struct acpi_madt_entry_header h;
    uint8_t acpi_id;
    uint8_t apic_id;
    uint32_t flags;
} PACKED;

struct madt_ioapic
{
    struct acpi_madt_entry_header h;
    uint8_t id;
    uint8_t reserved;
    uint32_t addr;
    uint32_t gsi_base;
} PACKED;

struct madt_irq_override
{
    struct acpi_madt_entry_header h;
    uint8_t bus;
    uint8_t irq;
    uint32_t gsi;
    uint16_t flags;
} PACKED;

void acpi_init(void *rsdp_virt);
void acpi_parse_madt(struct acpi_madt *madt);
void acpi_parse_fadt(void *fadt);
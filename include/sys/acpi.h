#pragma once

#include <stdint.h>
#include <system.h>

#define MAX_CPUS 32
#define MAX_IOAPICS 8
#define MAX_ISO 32
#define MAX_LAPIC_NMIS 16

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

#define MADT_TYPE_LAPIC              0
#define MADT_TYPE_IOAPIC             1
#define MADT_TYPE_ISO                2
#define MADT_TYPE_LAPIC_NMI          4
#define MADT_TYPE_LAPIC_ADDR_OVERRIDE 5

struct MADTLocalAPIC
{
    uint8_t type;
    uint8_t length;
    uint8_t AcpiProcessorId;
    uint8_t ApicId;
    uint32_t Flags;
} PACKED;

#define MADT_LAPIC_FLAG_ENABLED        0x1
#define MADT_LAPIC_FLAG_ONLINE_CAPABLE 0x2

struct MADTIOAPIC
{
    uint8_t type;
    uint8_t length;
    uint8_t IoApicId;
    uint8_t reserved;
    uint32_t IoApicAddress;
    uint32_t GlobalSystemInterruptBase;
};

struct MADTISO
{
    uint8_t type;
    uint8_t length;
    uint8_t BusSource;
    uint8_t IrqSource;
    uint32_t GlobalSystemInterrupt;
    uint16_t Flags;
} ;

struct MADTLapicNMI
{
    uint8_t type;
    uint8_t length;
    uint8_t AcpiProcessorId;
    uint16_t Flags;
    uint8_t Lint;
};

struct MADTLapicAddrOverride
{
    uint8_t type;
    uint8_t length;
    uint16_t reserved;
    uint64_t LocalApicAddress;
};

struct ACPIGenericAddress
{
    uint8_t AddressSpaceID;
    uint8_t RegisterBitWIdth;
    uint8_t RegisterBitOffset;
    uint8_t AccessSize;
    uint64_t Address;
};

struct HPET
{
    struct ACPISDTHeader h;
    uint32_t EventTimerBlockID;
    struct ACPIGenericAddress address;
    uint8_t HPETNumber;
    uint16_t MinimumTick;
    uint8_t PageProtection;
};

struct acpi_cpu_info
{
    uint8_t acpi_id;
    uint8_t apic_id;
    uint32_t flags;
};

struct acpi_ioapic_info
{
    uint8_t id;
    uint32_t gsi_base;
    uint64_t phys_addr;
};

struct acpi_iso_info
{
    uint8_t bus;
    uint8_t irq;
    uint32_t gsi;
    uint16_t flags;
};

struct acpi_lapic_nmi_info
{
    uint8_t acpi_id;
    uint8_t lint;
    uint16_t flags;
};

void acpi_init(void);

// lookup by 4 letter sig
struct ACPISDTHeader *acpi_find_table(const char sig[4]);

uint64_t acpi_get_lapic_base(void);

uint64_t acpi_get_hpet_base(void);

size_t acpi_get_cpu_count(void);
const struct acpi_cpu_info *acpi_get_cpus(void);

size_t acpi_get_ioapic_count(void);
const struct acpi_ioapic_info *acpi_get_ioapics(void);

size_t acpi_get_iso_count(void);
const struct acpi_iso_info *acpi_get_isos(void);

size_t acpi_get_lapic_nmi_count(void);
const struct acpi_lapic_nmi_info *acpi_get_lapic_nmis(void);

void acpi_selftest(void);
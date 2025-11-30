#include <stdint.h>
#include <stddef.h>

#include <limine.h>
#include <system.h>
#include <acpi.h>

static struct XSDP_t        *g_xsdp = NULL;
static struct XSDT          *g_xsdt = NULL;
static struct RSDT          *g_rsdt = NULL;
static struct MADT          *g_madt = NULL;
static struct FADT          *g_fadt = NULL;
static struct ACPISDTHeader *g_ssdt = NULL;

static uint64_t g_lapic_base_phys = 0;

static struct HPET *g_hpet = NULL;
static uint64_t g_hpet_base_phys = 0;

static struct acpi_cpu_info         g_cpus[MAX_CPUS];
static size_t                       g_cpu_count = 0;

static struct acpi_ioapic_info      g_ioapics[MAX_IOAPICS];
static size_t                       g_ioapic_count = 0;

static struct acpi_iso_info         g_isos[MAX_ISO];
static size_t                       g_iso_count = 0;

static struct acpi_lapic_nmi_info   g_lapic_nmis[MAX_LAPIC_NMIS];
static size_t                       g_lapic_nmi_count = 0;

__attribute__((used, section(".limine_requests")))
struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST_ID,
    .revision = 0,
};

static uint8_t acpi_checksum(const void *tbl, size_t len)
{
    const uint8_t *p = tbl;
    uint8_t sum = 0;
    for (size_t i = 0; i < len; i++)
        sum = (uint8_t)(sum + p[i]);
    return sum;
}

static int acpi_validate_rsdp(struct XSDP_t *xsdp)
{
    if (acpi_checksum(xsdp, 20) != 0)
        return 0;

    if (xsdp->Revision >= 2 && xsdp->Length != 0) {
        if (acpi_checksum(xsdp, xsdp->Length) != 0)
            return 0;
    }

    static const char rsd_sig[8] = { 'R','S','D',' ','P','T','R',' ' };
    if (memcmp(xsdp->Signature, rsd_sig, 8) != 0)
        return 0;

    return 1;
}

struct ACPISDTHeader *acpi_find_table(const char sig[4])
{
    // Prefer XSDT
    if (g_xsdt) {
        uint32_t length = g_xsdt->h.Length;
        uint32_t entry_count = (length - sizeof(struct ACPISDTHeader)) / sizeof(uint64_t);

        for (uint32_t i = 0; i < entry_count; i++) {
            uintptr_t phys = (uintptr_t)g_xsdt->entries[i];
            struct ACPISDTHeader *hdr =
                (struct ACPISDTHeader *)ACPI_PHYS_TO_VIRT(phys);

            if (memcmp(hdr->Signature, sig, 4) == 0) {
                // Optional: verify checksum
                if (acpi_checksum(hdr, hdr->Length) == 0) {
                    return hdr;
                } else {
                    kprintf("ACPI: table %.4s at %p has bad checksum\n",
                            hdr->Signature, (void *)phys);
                }
            }
        }
    }

    // Fallback via RSDT (32-bit entries)
    if (g_rsdt) {
        uint32_t length = g_rsdt->h.Length;
        uint32_t entry_count = (length - sizeof(struct ACPISDTHeader)) / sizeof(uint32_t);

        for (uint32_t i = 0; i < entry_count; i++) {
            uintptr_t phys = (uintptr_t)g_rsdt->entries[i];
            struct ACPISDTHeader *hdr =
                (struct ACPISDTHeader *)ACPI_PHYS_TO_VIRT(phys);

            if (memcmp(hdr->Signature, sig, 4) == 0) {
                if (acpi_checksum(hdr, hdr->Length) == 0) {
                    return hdr;
                } else {
                    kprintf("ACPI: table %.4s at %p has bad checksum\n",
                            hdr->Signature, (void *)phys);
                }
            }
        }
    }

    return NULL;
}

static void acpi_parse_madt(void)
{
    if (!g_madt)
        return;

    g_lapic_base_phys = (uint64_t)g_madt->LocalApicAddress;

    uint8_t *ptr = g_madt->entries;
    uint8_t *end = (uint8_t *)g_madt + g_madt->h.Length;

    while (ptr + 2 <= end) {
        uint8_t type   = ptr[0];
        uint8_t length = ptr[1];

        if (length < 2) {
            kprintf("[ACPI/MADT] Corrupt entry: length=%u\n", length);
            break;
        }
        if (ptr + length > end) {
            kprintf("[ACPI/MADT] Entry overruns MADT length, stopping\n");
            break;
        }

        switch (type) {

        case MADT_TYPE_LAPIC: {
            if (length < sizeof(struct MADTLocalAPIC)) {
                kprintf("[ACPI/MADT] LAPIC entry too short (len=%u)\n", length);
                break;
            }

            struct MADTLocalAPIC *lapic = (struct MADTLocalAPIC *)ptr;

            if (lapic->Flags & MADT_LAPIC_FLAG_ENABLED) {
                kprintf("[ACPI/MADT] CPU: ACPI=%u APIC=%u flags=0x%08x\n",
                        lapic->AcpiProcessorId,
                        lapic->ApicId,
                        lapic->Flags);

                if (g_cpu_count < MAX_CPUS) {
                    g_cpus[g_cpu_count].acpi_id = lapic->AcpiProcessorId;
                    g_cpus[g_cpu_count].apic_id = lapic->ApicId;
                    g_cpus[g_cpu_count].flags   = lapic->Flags;
                    g_cpu_count++;
                } else {
                    kprintf("[ACPI/MADT] CPU table full, skipping\n");
                }
            } else {
                kprintf("[ACPI/MADT] CPU (ACPI=%u APIC=%u) disabled, skipping\n",
                        lapic->AcpiProcessorId,
                        lapic->ApicId);
            }
        } break;

        case MADT_TYPE_IOAPIC: {
            if (length < sizeof(struct MADTIOAPIC)) {
                kprintf("[ACPI/MADT] IOAPIC entry too short (len=%u)\n", length);
                break;
            }

            struct MADTIOAPIC *io = (struct MADTIOAPIC *)ptr;

            kprintf("[ACPI/MADT] IOAPIC: id=%u addr=0x%08x gsi_base=%u\n",
                    io->IoApicId,
                    io->IoApicAddress,
                    io->GlobalSystemInterruptBase);

            if (g_ioapic_count < MAX_IOAPICS) {
                g_ioapics[g_ioapic_count].id       = io->IoApicId;
                g_ioapics[g_ioapic_count].gsi_base = io->GlobalSystemInterruptBase;
                g_ioapics[g_ioapic_count].phys_addr =
                    (uint64_t)io->IoApicAddress;
                g_ioapic_count++;
            } else {
                kprintf("[ACPI/MADT] IOAPIC table full, skipping\n");
            }
        } break;

        case MADT_TYPE_ISO: {
            if (length < sizeof(struct MADTISO)) {
                kprintf("[ACPI/MADT] ISO entry too short (len=%u)\n", length);
                break;
            }

            struct MADTISO *iso = (struct MADTISO *)ptr;

            kprintf("[ACPI/MADT] ISO: bus=%u irq=%u -> GSI=%u flags=0x%04x\n",
                    iso->BusSource,
                    iso->IrqSource,
                    iso->GlobalSystemInterrupt,
                    iso->Flags);

            if (g_iso_count < MAX_ISO) {
                g_isos[g_iso_count].bus   = iso->BusSource;
                g_isos[g_iso_count].irq   = iso->IrqSource;
                g_isos[g_iso_count].gsi   = iso->GlobalSystemInterrupt;
                g_isos[g_iso_count].flags = iso->Flags;
                g_iso_count++;
            } else {
                kprintf("[ACPI/MADT] ISO table full, skipping\n");
            }
        } break;

        case MADT_TYPE_LAPIC_NMI: {
            if (length < sizeof(struct MADTLapicNMI)) {
                kprintf("[ACPI/MADT] LAPIC NMI entry too short (len=%u)\n", length);
                break;
            }

            struct MADTLapicNMI *nmi = (struct MADTLapicNMI *)ptr;

            kprintf("[ACPI/MADT] LAPIC NMI: cpu=%s flags=0x%04x LINT%u\n",
                    (nmi->AcpiProcessorId == 0xFF) ? "ALL" : "per-CPU",
                    nmi->Flags,
                    nmi->Lint);

            if (g_lapic_nmi_count < MAX_LAPIC_NMIS) {
                g_lapic_nmis[g_lapic_nmi_count].acpi_id = nmi->AcpiProcessorId;
                g_lapic_nmis[g_lapic_nmi_count].lint    = nmi->Lint;
                g_lapic_nmis[g_lapic_nmi_count].flags   = nmi->Flags;
                g_lapic_nmi_count++;
            } else {
                kprintf("[ACPI/MADT] LAPIC NMI table full, skipping\n");
            }
        } break;

        case MADT_TYPE_LAPIC_ADDR_OVERRIDE: {
            if (length < sizeof(struct MADTLapicAddrOverride)) {
                kprintf("[ACPI/MADT] LAPIC override entry too short (len=%u)\n", length);
                break;
            }

            struct MADTLapicAddrOverride *ovr =
                (struct MADTLapicAddrOverride *)ptr;

            g_lapic_base_phys = ovr->LocalApicAddress;

            kprintf("[ACPI/MADT] LAPIC address override: new base=0x%016llx\n",
                    (unsigned long long)g_lapic_base_phys);
        } break;

        default:
            kprintf("[ACPI/MADT] Unknown entry type=%u len=%u, skipping\n",
                    type, length);
            break;
        }

        ptr += length;
    }

    kprintf("[ACPI/MADT] Final LAPIC base=0x%016llx\n",
            (unsigned long long)g_lapic_base_phys);
}

/**
 * i DESPERATLY need to clean up these tiny static getters
 * note for future devs; replace these 
 */

uint64_t acpi_get_lapic_base(void)
{
    return g_lapic_base_phys;
}

uint64_t acpi_get_hpet_base(void)
{
    return g_hpet_base_phys;
}

size_t acpi_get_cpu_count(void)
{
    return g_cpu_count;
}

const struct acpi_cpu_info *acpi_get_cpus(void)
{
    return g_cpus;
}

size_t acpi_get_ioapic_count(void)
{
    return g_ioapic_count;
}

const struct acpi_ioapic_info *acpi_get_ioapics(void)
{
    return g_ioapics;
}

size_t acpi_get_iso_count(void)
{
    return g_iso_count;
}

const struct acpi_iso_info *acpi_get_isos(void)
{
    return g_isos;
}

size_t acpi_get_lapic_nmi_count(void)
{
    return g_lapic_nmi_count;
}

const struct acpi_lapic_nmi_info *acpi_get_lapic_nmis(void)
{
    return g_lapic_nmis;
}

void acpi_selftest(void)
{
    size_t cpu_count        = acpi_get_cpu_count();
    size_t ioapic_count     = acpi_get_ioapic_count();
    size_t iso_count        = acpi_get_iso_count();
    size_t lapic_nmi_count  = acpi_get_lapic_nmi_count();

    kprintf("[ACPI/TEST] cpus=%u ioapics=%u iso=%u lapic_nmis=%u\n",
            (unsigned)cpu_count,
            (unsigned)ioapic_count,
            (unsigned)iso_count,
            (unsigned)lapic_nmi_count);

    const struct acpi_cpu_info *cpus        = acpi_get_cpus();
    const struct acpi_ioapic_info *ios      = acpi_get_ioapics();
    const struct acpi_iso_info *isos        = acpi_get_isos();
    const struct acpi_lapic_nmi_info *nmis  = acpi_get_lapic_nmis();

    for (size_t i = 0; i < cpu_count; ++i) {
        kprintf("[ACPI/TEST] CPU[%u]: ACPI=%u APIC=%u flags=0x%08x\n",
                (unsigned)i,
                cpus[i].acpi_id,
                cpus[i].apic_id,
                cpus[i].flags);
    }

    for (size_t i = 0; i < ioapic_count; ++i) {
        kprintf("[ACPI/TEST] IOAPIC[%u]: id=%u gsi_base=%u phys=0x%016llx\n",
                (unsigned)i,
                ios[i].id,
                ios[i].gsi_base,
                (unsigned long long)ios[i].phys_addr);
    }

    for (size_t i = 0; i < iso_count; ++i) {
        kprintf("[ACPI/TEST] ISO[%u]: bus=%u irq=%u gsi=%u flags=0x%04x\n",
                (unsigned)i,
                isos[i].bus,
                isos[i].irq,
                isos[i].gsi,
                isos[i].flags);
    }

    for (size_t i = 0; i < lapic_nmi_count; ++i) {
        kprintf("[ACPI/TEST] LAPIC_NMI[%u]: acpi_id=%u lint=%u flags=0x%04x\n",
                (unsigned)i,
                nmis[i].acpi_id,
                nmis[i].lint,
                nmis[i].flags);
    }
}
void acpi_init(void)
{
    if (!rsdp_request.response) {
        panic("[ACPI] Limine didnt supply an RSDP\n");
    }

    uintptr_t rsdp_phys = (uintptr_t)rsdp_request.response->address;

    g_xsdp = (struct XSDP_t *)ACPI_PHYS_TO_VIRT(rsdp_phys);

    if (!acpi_validate_rsdp(g_xsdp)) {
        panic("[ACPI] RSDP checksum/signature invalid\n");
    }

    kprintf("[ACPI] REVISION: %u\n",
            g_xsdp->Revision);

    // Require XSDT on this platform
    if (g_xsdp->Revision >= 2 && g_xsdp->XsdtAddress != 0) {
        uintptr_t xsdt_phys = (uintptr_t)g_xsdp->XsdtAddress;
        g_xsdt = (struct XSDT *)ACPI_PHYS_TO_VIRT(xsdt_phys);

        if (acpi_checksum(g_xsdt, g_xsdt->h.Length) != 0) {
            panic("[ACPI] XSDT checksum invalid\n");
        } else {
            kprintf("[ACPI] XSDT found at phys=%p, len=%u\n",
                    (void *)xsdt_phys, g_xsdt->h.Length);
        }
    } else {
        panic("[ACPI] No valid 64-bit XSDT pointer in RSDP\n");
    }

    if (!g_xsdt) {
        // This is redundant if panic() never returns; keep if panic might return.
        panic("[ACPI] No valid 64-bit table pointer, system halted\n");
    }

    g_madt = (struct MADT *)acpi_find_table("APIC");
    g_fadt = (struct FADT *)acpi_find_table("FACP");
    g_ssdt = acpi_find_table("SSDT");
    g_hpet = (struct HPET *)acpi_find_table("HPET");

    if (g_madt) {
        kprintf("[ACPI] MADT found, LAPIC=0x%08x, Flags=0x%08x\n",
                g_madt->LocalApicAddress, g_madt->Flags);
        acpi_parse_madt();
    } else {
        kprintf("[ACPI] MADT not found\n");
    }

    if (g_fadt) {
        kprintf("[ACPI] FADT found (FACP)\n");
    } else {
        kprintf("[ACPI] FADT not found\n");
    }

    if (g_ssdt) {
        kprintf("[ACPI] SSDT found\n");
    } else {
        kprintf("[ACPI] SSDT not found\n");
    }

    if (g_hpet) {
        if (g_hpet->address.AddressSpaceID == 0) { // system memory
            g_hpet_base_phys = g_hpet->address.Address;
            kprintf("[ACPI] HPET found at phys=0x%016llx\n",
                    (unsigned long long)g_hpet_base_phys);
        } else {
            kprintf("[ACPI] HPET address space unsupported (%u)\n",
                    g_hpet->address.AddressSpaceID);
        }
    } else {
        kprintf("[ACPI] HPET not found\n");
    }
}
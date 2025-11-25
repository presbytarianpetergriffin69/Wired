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

    if (g_madt) {
        kprintf("[ACPI] MADT found, LAPIC=0x%08x, Flags=0x%08x\n",
                g_madt->LocalApicAddress, g_madt->Flags);
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

    if (!g_madt && !g_fadt && !g_ssdt) {
        panic("[ACPI] No valid MADT/FADT/SSDT tables found, system halted\n");
    }
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
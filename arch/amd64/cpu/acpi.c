#include <stdint.h>
#include <stddef.h>

#include <limine.h>
#include <system.h>
#include <acpi.h>
#include <hpet.h>

XSDP_t *g_xsdp = NULL;
uint64_t acpi_hhdm_offset = 0;

ACPISDTHeader *g_xsdt = NULL;
ACPISDTHeader *g_rsdt = NULL;

static ACPISDTHeader *g_root_sdt = NULL;
static int g_acpi_is_xsdt = 0;

USED_SECTION(".limine_requests")
static volatile uint64_t limine_base_revision[] =
    LIMINE_BASE_REVISION(4);

USED_SECTION(".limine_requests")
static volatile struct limine_hhdm_request hhdm_request =
{
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0
};

USED_SECTION(".limine_requests")
static volatile struct limine_rsdp_request rsdp_request =
{
    .id = LIMINE_RSDP_REQUEST_ID,
    .revision = 0
};

USED_SECTION(".limine_requests_start")
static volatile uint64_t limine_requests_start_marker[] =
    LIMINE_REQUESTS_START_MARKER;

USED_SECTION(".limine_requests_end")
static volatile uint64_t limine_requests_end_marker[] =
    LIMINE_REQUESTS_END_MARKER;

static int acpi_checksum(const void *ptr, size_t len)
{
    const uint8_t *p = ptr;
    uint8_t sum = 0;
    for (size_t i = 0; i < len; i++)
        sum += p[i];
    return sum == 0;
}

static inline void *phys_to_virt(uint64_t phys)
{
    return (void *)(phys + acpi_hhdm_offset);
}

void acpi_enum_tables(void)
{
    if (!g_root_sdt)
        panic("[ACPI] Root SDT Missing\n");
    
    uint32_t entries = 0;

    if (g_acpi_is_xsdt) {
        entries = (g_root_sdt->Length - sizeof(ACPISDTHeader)) / 8;
    } else {
        entries = (g_root_sdt->Length - sizeof(ACPISDTHeader)) / 4;
    }

    kprintf("[ACPI] Root SDT Contains %u entries\n", entries);

    uint8_t *entry_ptr = (uint8_t *)(g_root_sdt + 1);

    for (uint32_t i = 0; i < entries; i++) {
        uint64_t phys = g_acpi_is_xsdt
            ? *(uint64_t *)(entry_ptr + i * 8)
            : *(uint32_t *)(entry_ptr + i * 4);

        ACPISDTHeader *hdr = (ACPISDTHeader *)phys_to_virt(phys);

    kprintf("[ACPI] Found Table: %c%c%c%c at %p len=%u\n",
            hdr->Signature[0],
            hdr->Signature[1],
            hdr->Signature[2],
            hdr->Signature[3],
            hdr,
            hdr->Length);
    }
}

void acpi_init(void)
{
    if (!LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision)) {
        panic("[ACPI] Limine base revision too old\n");
    }

    if (!rsdp_request.response || !rsdp_request.response->address) {
        panic("[ACPI] No XSDP Table from Limine\n");
    }

    if (hhdm_request.response) {
        acpi_hhdm_offset = hhdm_request.response->offset;
    } else {
        acpi_hhdm_offset = 0;
        kprintf("[ACPI] No HHDM Response\n");
    }   

    g_xsdp = (XSDP_t *)rsdp_request.response->address;

    kprintf("[ACPI] hhdm_request.response = %p\n", hhdm_request.response);
    kprintf("[ACPI] HHDM offset = 0x%016lx\n", (unsigned long)acpi_hhdm_offset);

    if (memcmp(g_xsdp->Signature, "RSD PTR ", 8) != 0) {
        panic("[ACPI] bad signature\n");
    }

    if (!acpi_checksum(g_xsdp, 20)) {
        panic("[ACPI] XSDP v1 checksum failed\n");
    }

    if (g_xsdp->Revision >= 2 && g_xsdp->Length >= sizeof(XSDP_t)) {
        if (!acpi_checksum(g_xsdp, g_xsdp->Length)) {
            panic("[ACPI] V2 struct error: extended checksum failed\n");
        }
    }

    kprintf("[ACPI] OEMID=%p Rev=%u XSDT=%p\n",
            g_xsdp->OEMID,
            g_xsdp->Revision,
            (unsigned long)g_xsdp->XsdtAddress);

    kprintf("[ACPI] XSDP loaded\n");

    if (g_xsdp->Revision >= 2 && g_xsdp->XsdtAddress != 0) {
        g_acpi_is_xsdt = 1;
        g_root_sdt = (ACPISDTHeader *)phys_to_virt(g_xsdp->XsdtAddress);
        kprintf("[ACPI] Using XSDT at %p\n", g_root_sdt);
    } else if (g_xsdp->RsdtAddress != 0) {
        g_acpi_is_xsdt = 0;
        g_root_sdt = (ACPISDTHeader *)phys_to_virt(g_xsdp->RsdtAddress);
        kprintf("[ACPI] Using RSDT at %p\n", g_root_sdt);
    } else {
        panic("[ACPI] No XSDT/RSDT pointer available\n");
    }

    kprintf("[ACPI] Root SDT %c%c%c%c len=%u\n",
            g_root_sdt->Signature[0],
            g_root_sdt->Signature[1],
            g_root_sdt->Signature[2],
            g_root_sdt->Signature[3],
            g_root_sdt->Length);

    if (!acpi_checksum(g_root_sdt, g_root_sdt->Length))
        panic("[ACPI] Root SDT Checksum failed\n");

    acpi_enum_tables();

    hpet_init();

    kprintf("acpi initialized\n");
}

ACPISDTHeader *acpi_find_table(const char sig[4])
{
    // 1) Try XSDT (64-bit entries)
    if (g_xsdt) {
        uint8_t *xsdt_bytes = (uint8_t *)g_xsdt;
        uint32_t length     = g_xsdt->Length;

        // entries start immediately after the header
        uintptr_t entries_addr = (uintptr_t)(xsdt_bytes + sizeof(ACPISDTHeader));
        size_t entry_count = (length - sizeof(ACPISDTHeader)) / 8; // 8 bytes per entry

        uint64_t *entries = (uint64_t *)entries_addr;

        for (size_t i = 0; i < entry_count; ++i) {
            uint64_t phys = entries[i];
            if (!phys)
                continue;

            ACPISDTHeader *hdr = (ACPISDTHeader *)ACPI_PHYS_TO_VIRT(phys);

            // Compare 4-byte signature manually (no libc needed)
            if (hdr->Signature[0] == sig[0] &&
                hdr->Signature[1] == sig[1] &&
                hdr->Signature[2] == sig[2] &&
                hdr->Signature[3] == sig[3]) {

                // Optional checksum check
                if (!acpi_checksum(hdr, hdr->Length)) {
                    kprintf("ACPI: table %.4s failed checksum\n", sig);
                    continue;
                }

                return hdr;
            }
        }
    }

    // 2) Try RSDT (32-bit entries)
    if (g_rsdt) {
        uint8_t *rsdt_bytes = (uint8_t *)g_rsdt;
        uint32_t length     = g_rsdt->Length;

        uintptr_t entries_addr = (uintptr_t)(rsdt_bytes + sizeof(ACPISDTHeader));
        size_t entry_count = (length - sizeof(ACPISDTHeader)) / 4; // 4 bytes per entry

        uint32_t *entries = (uint32_t *)entries_addr;

        for (size_t i = 0; i < entry_count; ++i) {
            uint32_t phys = entries[i];
            if (!phys)
                continue;

            ACPISDTHeader *hdr = (ACPISDTHeader *)ACPI_PHYS_TO_VIRT(phys);

            if (hdr->Signature[0] == sig[0] &&
                hdr->Signature[1] == sig[1] &&
                hdr->Signature[2] == sig[2] &&
                hdr->Signature[3] == sig[3]) {

                if (!acpi_checksum(hdr, hdr->Length)) {
                    kprintf("ACPI: table %.4s failed checksum (RSDT)\n", sig);
                    continue;
                }

                return hdr;
            }
        }
    }

    return NULL;
}
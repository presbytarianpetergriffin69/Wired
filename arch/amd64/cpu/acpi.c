#include <stdint.h>
#include <stddef.h>

#include <limine.h>
#include <system.h>
#include <acpi.h>

XSDP_t *g_xsdp = NULL;

uint64_t acpi_hhdm_offset = 0;

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

    uint64_t xsdp_phys = (uint64_t)rsdp_request.response->address;
    g_xsdp = (XSDP_t *)(xsdp_phys + acpi_hhdm_offset);

    kprintf("[ACPI] XSDP phys=0x%016lx virt=%p\n",
            xsdp_phys, g_xsdp);

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

    kprintf("[ACPI] OEMID=%.6s Rev=%u XSDT=%p\n",
            g_xsdp->OEMID,
            g_xsdp->Revision,
            (unsigned long)g_xsdp->XsdtAddress);

    kprintf("[ACPI] XSDP loaded\n");
} 
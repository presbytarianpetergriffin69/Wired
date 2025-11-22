#include <stdint.h>
#include <stddef.h>

#include <limine.h>
#include <system.h>
#include <acpi.h>

XSDP_t *g_xsdp = NULL;
uint64_t acpi_hhdm_offset = 0;
ACPISDTHeader *g_xsdt = NULL;
ACPISDTHeader *g_rsdt = NULL;
uint64_t root_phys = 0;

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

static ACPISDTHeader *g_root_sdt = NULL;
static int g_acpi_is_xsdt = 0;

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
    
    g_xsdp = (XSDP_t *)rsdp_request.response->address;   // already virtual
    uint64_t xsdp_phys = (uint64_t)g_xsdp - acpi_hhdm_offset;

    kprintf("[ACPI] hhdm_request.response = %p\n", hhdm_request.response);
    if (hhdm_request.response)
        kprintf("[ACPI] HHDM offset = 0x%016lx\n", (unsigned long)hhdm_request.response->offset);
    else
        kprintf("[ACPI] HHDM offset not provided\n");

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

    ACPISDTHeader *root = NULL;

    if (g_xsdp->Revision >= 2 && g_xsdp->XsdtAddress != 0) {
        g_xsdt = (ACPISDTHeader *)phys_to_virt(g_xsdp->XsdtAddress);
        root = g_xsdt;
        kprintf("[ACPI] Using XSDT at %p\n", g_xsdt);
    } else if (g_xsdp->RsdtAddress != 0) {
        g_rsdt = (ACPISDTHeader *)phys_to_virt(g_xsdp->RsdtAddress);
        root = g_rsdt;
        kprintf("[ACPI] Using RSDT at %p\n", g_rsdt);
    } else {
        panic("[ACPI] No XSDT/RSDT Pointer available\n");
    }

    if (!acpi_checksum(root, root->Length))
        panic("[ACPI] Root SDT Checksum failed\n");

    g_root_sdt = (ACPISDTHeader *)phys_to_virt(root_phys);

    kprintf("[ACPI] Root SDT %.4s len=%u\n",
            g_root_sdt->Signature, root->Length);

    kprintf("acpi initialized\n");
} 
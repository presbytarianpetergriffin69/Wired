#include <stdint.h>
#include <stddef.h>
#include <limine.h>
#include <system.h>
#include <acpi.h>

static struct acpi_rsdp *g_rsdp;
static struct acpi_sdt_header *g_xsdt;
static struct acpi_sdt_header *g_rsdt;
static uint64_t g_hhdm;

USED_SECTION(".limine_requests")
static volatile struct limine_rsdp_request rsdp_request =
{
    .id = LIMINE_RSDP_REQUEST_ID,
    .revision = 0
};

USED_SECTION(".limine_requests")
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(4);

USED_SECTION(".limine_requests")
static volatile struct limine_hhdm_request hhdm_request =
{
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0
};

USED_SECTION(".limine_requests")
static volatile struct limine_memmap_request memmap_request = 
{
    .id = LIMINE_MEMMAP_REQUEST_ID,
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
    for (size_t i = 0; i < len; i++) sum += p[i];
    return sum == 0;
}

static struct acpi_sdt_header *phys_to_sdt(uint64_t phys)
{
    return (struct acpi_sdt_header *)(phys + g_hhdm);
}

uint64_t acpi_hhdm_offset = 0;

void acpi_init(void)
{
    if (!LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision)) {
        panic("limine base revision too old");
    }

    if (!rsdp_request.response || !rsdp_request.response->address) {
        panic("mo limine description pointer");
    }

    if (!hhdm_request.response) {
        panic("mo hhdm from limine");
    }

    g_hhdm = hhdm_request.response->offset;
    kprintf("[ACPI] HHDM Offset = %p\n");

    uint64_t rsdp_phys = (uint64_t)rsdp_request.response->address;
    struct acpi_rsdp *rsdp =
        (struct acpi_rsdp *)(rsdp_phys + acpi_hhdm_offset);

    kprintf("[ACPI] Proceeding with what we have\n");
}

struct acpi_sdt_header *acpi_find_table(const char sig[4])
{
    if (g_xsdt) {
        uint32_t n = (g_xsdt->length - sizeof(*g_xsdt)) / 8;
        uint64_t *entries = (uint64_t *)(g_xsdt + 1);

        for (uint32_t i = 0; i < n; i++) {
            struct acpi_sdt_header *h = phys_to_sdt(entries[i]);
            if (memcmp(h->signature, sig, 4) == 0) {
                if (acpi_checksum(h, h->length))
                    return h;
            }
        }
    }

    if (g_rsdt) {
        uint32_t n = (g_rsdt->length - sizeof(*g_rsdt)) / 4;
        uint32_t *entries = (uint32_t *)(g_rsdt + 1);

        for (uint32_t i = 0; i < n; i++) {
            struct acpi_sdt_header *h = phys_to_sdt(entries[i]);
            if (memcmp(h->signature, sig, 4) == 0) {
                if (acpi_checksum(h, h->length))
                    return h;
            }
        }

    }

    return NULL;
}
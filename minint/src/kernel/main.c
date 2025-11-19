#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include <console.h>
#include <font8x16.h>
#include <system.h>

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(4);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] =
LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] =
LIMINE_REQUESTS_END_MARKER;

void kmain(void)
{
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        halt();
    }

    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        halt();
    }

    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];

    console_init(framebuffer);

    console_print("Welcome to the Wired\n");

    panic("test panic");

    halt();
}
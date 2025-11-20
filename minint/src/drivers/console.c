#include "console.h"
#include "font8x16.h"

static struct limine_framebuffer *fb;
static uint32_t *fb_ptr;

static uint32_t fb_width;
static uint32_t fb_height;
static uint32_t fb_pitch;

static size_t cursor_x = 0;
static size_t cursor_y = 0;

#define FONT_W 8
#define FONT_H 16

static inline void putpixel(uint32_t x, uint32_t y, uint32_t color)
{
    fb_ptr[y * (fb_pitch / 4) + x] = color;
}

static void draw_char(char c, uint32_t x, uint32_t y, uint32_t fg, uint32_t bg)
{
    const uint8_t *glyph = font8x16[(uint8_t)c];

    for (size_t row = 0; row < FONT_H; row++) {
        uint8_t bits = glyph[row];
        for (size_t col = 0; col < FONT_W; col++) {
            uint32_t color = (bits & (1 << (7 - col))) ? fg : bg;
            putpixel(x + col, y + row, color);
        }
    }
}

static void scroll(void)
{
    const size_t stride = fb_pitch / 4;
    const size_t row_pixels = stride * FONT_H;

    for (size_t y = 0; y < fb_height - FONT_H; y++) {
        fb_ptr[y * stride] = fb_ptr[(y + FONT_H) * stride];
        for (size_t x = 0; x < fb_width; x++)
            fb_ptr[y * stride + x] = fb_ptr[(y + FONT_H) * stride + x];
    }

    for (size_t y = fb_height - FONT_H; y < fb_height; y++)
        for (size_t x = 0; x < fb_width; x++)
            putpixel(x, y, 0x000000);
}

void console_init(struct limine_framebuffer *framebuffer)
{
    fb = framebuffer;
    fb_ptr = (uint32_t*)framebuffer->address;

    fb_width = fb->width;
    fb_height = fb->height;
    fb_pitch = fb->pitch;

    cursor_x = cursor_y = 0;
    console_clear();
}

void console_clear(void) 
{
    for (size_t y = 0; y < fb_height; y++)
        for (size_t x = 0; x < fb_width; x++)
            putpixel(x, y, 0x000000);

    cursor_x = cursor_y = 0;
}

void console_putc(char c)
{
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        goto check_scroll;
    }

    draw_char(c, cursor_x * FONT_W, cursor_y * FONT_H, 0xffffff, 0x000000);

    cursor_x++;
    if (cursor_x * FONT_W >= fb_width) {
        cursor_x = 0;
        cursor_y++;
    }

check_scroll:
    if ((cursor_y + 1) * FONT_H >= fb_height) {
        scroll();
        cursor_y--;
    }
}

void console_print(const char *s)
{
    while (*s) {
        console_putc(*s++);
    }
}

void console_fill(uint32_t color)
{
    uint32_t *p = fb->address;
    size_t pixels = (fb->pitch / 4) * fb->height;

    for (size_t i = 0; i < pixels; i++)
        p[i] = color;

    cursor_x = cursor_y = 0;
}

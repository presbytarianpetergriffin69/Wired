#include "console.h"
#include "font8x16.h"
#include <limine.h>
#include <stdint.h>
#include <stddef.h>

#define CONSOLE_FG 0xFFFFFFFF
#define CONSOLE_BG 0x00CC0000

// Framebuffer state
static struct limine_framebuffer *fb;
static uint32_t *fb_ptr;

static uint32_t fb_width;
static uint32_t fb_height;
static uint32_t fb_pitch;

static size_t cursor_x = 0;
static size_t cursor_y = 0;

#define FONT_W 8
#define FONT_H 16

typedef struct {
    uint16_t x;
    uint16_t y;
    uint8_t  state;   // 0 = off, 1 = dim, 2 = bright
} star_t;

#define MAX_STARS   600
#define STAR_DIM    0x808080
#define STAR_BRIGHT 0xFFFFFF

static star_t stars[MAX_STARS];

static inline void putpixel(uint32_t x, uint32_t y, uint32_t color)
{
    if (x >= fb_width || y >= fb_height) return;
    fb_ptr[y * (fb_pitch / 4) + x] = color;
}

static inline uint32_t getpixel(uint32_t x, uint32_t y)
{
    if (x >= fb_width || y >= fb_height) return 0;
    return fb_ptr[y * (fb_pitch / 4) + x];
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

    for (size_t y = 0; y < fb_height - FONT_H; y++) {
        for (size_t x = 0; x < fb_width; x++)
            fb_ptr[y * stride + x] = fb_ptr[(y + FONT_H) * stride + x];
    }

    for (size_t y = fb_height - FONT_H; y < fb_height; y++)
        for (size_t x = 0; x < fb_width; x++)
            putpixel(x, y, 0x000000);

    for (int i = 0; i < MAX_STARS; i++) {
        if (stars[i].y >= FONT_H)
            stars[i].y -= FONT_H;
        else
            stars[i].y = fb_height - (FONT_H - stars[i].y);
    }
    
    for (int i = 0; i < MAX_STARS; i++) {
        putpixel(stars[i].x, stars[i].y, STAR_DIM);
    }
}

void console_init(struct limine_framebuffer *framebuffer)
{
    fb = framebuffer;
    fb_ptr = (uint32_t*)framebuffer->address;

    fb_width  = fb->width;
    fb_height = fb->height;
    fb_pitch  = fb->pitch;

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
        cursor_y += FONT_H;

        // scroll if we moved past bottom
        if (cursor_y + FONT_H > fb_height) {
            scroll();
            cursor_y -= FONT_H;
        }

        return;
    }

    const uint8_t *glyph = font8x16[(uint8_t)c];

    for (int y = 0; y < FONT_H; y++) {
        uint8_t row = glyph[y];

        for (int x = 0; x < FONT_W; x++) {
            size_t fx = cursor_x + x;
            size_t fy = cursor_y + y;

            if (fx >= fb_width || fy >= fb_height)
                continue;

            // ALWAYS paint background red
            fb_ptr[fy * (fb_pitch / 4) + fx] = CONSOLE_BG;

            // Draw glyph in white
            if (row & (1 << (7 - x))) {
                fb_ptr[fy * (fb_pitch / 4) + fx] = CONSOLE_FG;
            }
        }
    }

    cursor_x += FONT_W;
    if (cursor_x + FONT_W >= fb_width) {
        cursor_x = 0;
        cursor_y += FONT_H;

        // scroll if we moved past bottom
        if (cursor_y + FONT_H > fb_height) {
            scroll();
            cursor_y -= FONT_H;
        }
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

static uint32_t rnd_state = 0x12345678;
static uint32_t rand32(void)
{
    rnd_state = rnd_state * 1664525u + 1013904223u;
    return rnd_state;
}

// Draw initial stars immediately
void starfield_init(void)
{
    for (int i = 0; i < MAX_STARS; i++) {
        stars[i].x = rand32() % fb_width;
        stars[i].y = rand32() % fb_height;
        stars[i].state = 1; // start dim

        putpixel(stars[i].x, stars[i].y, STAR_DIM);
        }
}
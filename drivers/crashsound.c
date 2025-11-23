#include <crashsound.h>
#include <io.h>
#include <stdint.h>

void beep(uint32_t freq) 
{
    if (!freq) return;

    uint32_t div = 1193180 / freq;

    // PIT channel 2: square wave
    outb(0x43, 0xB6);
    outb(0x42, div & 0xFF);
    outb(0x42, div >> 8);

    // enable speaker
    uint8_t tmp = inb(0x61);
    outb(0x61, tmp | 3);
}

void speaker_off()
{
    uint8_t tmp = inb(0x61);
    outb(0x61, tmp & ~3);
}

void delay_loop(uint32_t ms) 
{
    const uint32_t DELAY_FACTOR = 350000;

    for (uint32_t i = 0; i < ms * DELAY_FACTOR; i++) {
        __asm__ volatile ("" ::: "memory");
    }
}


void crashsound(void) 
{
    enum {
        F4_HZ  = 347,  // 346.68
        C5_HZ  = 700,  // 700
        A4_HZ  = 437,  // 436.68
        Bb4_HZ = 520,  // 520.35
        B4_HZ  = 460,
        D4_HZ  = 355
    };

    struct {
        uint32_t freq;
        uint32_t dur_ms;
    } notes[] = {
        { F4_HZ,  600 },
        { A4_HZ,  600 },
        { Bb4_HZ, 600 }, 
        { C5_HZ,  1500 }, 

        { A4_HZ, 600 },
        { B4_HZ,  600 },
        { A4_HZ,  600 },
        { D4_HZ,  1500 },
    };

    for (int i = 0; i < 8; i++) {
        beep(notes[i].freq);
        delay_loop(notes[i].dur_ms);
        speaker_off();
        delay_loop(40);   // gap between notes
    }
}
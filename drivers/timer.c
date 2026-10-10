/* 8253/8254 Programmable Interval Timer on IRQ0. */
#include "kernel.h"

static volatile uint32_t ticks;
static uint32_t freq;

static void on_tick(regs_t *r) { (void)r; ticks++; }

void timer_init(uint32_t hz) {
    freq = hz;
    uint32_t divisor = 1193182 / hz;      /* PIT base clock is 1.193182 MHz */
    outb(0x43, 0x36);                     /* channel 0, lo/hi byte, mode 3 (square wave) */
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
    irq_register(0, on_tick);
}

uint32_t timer_ticks(void) { return ticks; }

void timer_sleep(uint32_t ms) {
    uint32_t end = ticks + (ms * freq + 999) / 1000;
    while (ticks < end) hlt();            /* sleep until the next interrupt */
}

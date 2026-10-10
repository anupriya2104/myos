/* 16550 UART on COM1 (0x3F8). Mirrors all console output to the serial port
 * and accepts shell input from it, so the OS can be driven headless
 * (qemu -serial stdio) and tested automatically. */
#include "kernel.h"

#define COM1 0x3F8

void serial_init(void) {
    outb(COM1 + 1, 0x00);   /* disable UART interrupts (we poll) */
    outb(COM1 + 3, 0x80);   /* DLAB on to set the baud divisor */
    outb(COM1 + 0, 0x03);   /* divisor 3 -> 38400 baud */
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);   /* 8 data bits, no parity, 1 stop bit */
    outb(COM1 + 2, 0xC7);   /* enable and clear FIFOs */
    outb(COM1 + 4, 0x0B);   /* DTR, RTS, OUT2 */
}

void serial_putc(char c) {
    if (c == '\n') serial_putc('\r');
    while (!(inb(COM1 + 5) & 0x20)) { }   /* wait until transmit holding reg is empty */
    outb(COM1, c);
}

int serial_getc_nonblock(void) {
    if (!(inb(COM1 + 5) & 0x01)) return -1;   /* no data ready */
    return inb(COM1);
}

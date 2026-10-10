/* VGA text-mode driver: 80x25 cells at physical 0xB8000, each cell is
 * (attribute << 8) | ascii. Handles scrolling and the hardware cursor. */
#include "kernel.h"

#define W 80
#define H 25
static volatile uint16_t *const vram = (uint16_t *)0xB8000;
static int row, col;
static uint8_t color = 0x07;

static void move_cursor(void) {
    uint16_t pos = row * W + col;
    outb(0x3D4, 0x0F); outb(0x3D5, pos & 0xFF);
    outb(0x3D4, 0x0E); outb(0x3D5, pos >> 8);
}

void vga_set_color(uint8_t fg, uint8_t bg) { color = (bg << 4) | (fg & 0x0F); }

void vga_clear(void) {
    for (int i = 0; i < W * H; i++) vram[i] = (color << 8) | ' ';
    row = col = 0;
    move_cursor();
}

static void scroll(void) {
    for (int i = 0; i < W * (H - 1); i++) vram[i] = vram[i + W];
    for (int i = W * (H - 1); i < W * H; i++) vram[i] = (color << 8) | ' ';
    row = H - 1;
}

void vga_putc(char c) {
    if (c == '\n') { col = 0; row++; }
    else if (c == '\r') { col = 0; }
    else if (c == '\b') {
        if (col > 0) col--;
        vram[row * W + col] = (color << 8) | ' ';
    } else if (c == '\t') { col = (col + 4) & ~3; }
    else {
        vram[row * W + col] = (color << 8) | (uint8_t)c;
        if (++col == W) { col = 0; row++; }
    }
    if (col >= W) { col = 0; row++; }
    if (row >= H) scroll();
    move_cursor();
}

void vga_init(void) { vga_set_color(7, 0); vga_clear(); }

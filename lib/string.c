/* Freestanding replacements for the bits of libc the kernel needs.
 * There is no libc in kernel space, so these are written by hand. */
#include "kernel.h"
#include <stdarg.h>

void *memset(void *d, int c, size_t n) {
    uint8_t *p = d;
    while (n--) *p++ = (uint8_t)c;
    return d;
}

void *memcpy(void *d, const void *s, size_t n) {
    uint8_t *a = d; const uint8_t *b = s;
    while (n--) *a++ = *b++;
    return d;
}

size_t strlen(const char *s) { size_t n = 0; while (s[n]) n++; return n; }

int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (uint8_t)*a - (uint8_t)*b;
}

int strncmp(const char *a, const char *b, size_t n) {
    while (n && *a && *a == *b) { a++; b++; n--; }
    return n ? (uint8_t)*a - (uint8_t)*b : 0;
}

int atoi(const char *s) {
    int sign = 1, v = 0;
    if (*s == '-') { sign = -1; s++; }
    while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0');
    return sign * v;
}

static void putc_all(char c) { vga_putc(c); serial_putc(c); }

static void print_num(uint32_t v, int base, int width, char pad) {
    char tmp[32]; int n = 0;
    do { tmp[n++] = "0123456789abcdef"[v % base]; v /= base; } while (v);
    while (n < width) tmp[n++] = pad;
    while (n) putc_all(tmp[--n]);
}

/* Supports %s %c %d %u %x %p %% with optional zero-pad width, e.g. %08x. */
void kprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    for (; *fmt; fmt++) {
        if (*fmt != '%') { putc_all(*fmt); continue; }
        fmt++;
        char pad = ' '; int width = 0;
        if (*fmt == '0') { pad = '0'; fmt++; }
        while (*fmt >= '0' && *fmt <= '9') width = width * 10 + (*fmt++ - '0');
        switch (*fmt) {
            case 's': { const char *s = va_arg(ap, const char *); while (*s) putc_all(*s++); break; }
            case 'c': putc_all((char)va_arg(ap, int)); break;
            case 'd': { int v = va_arg(ap, int);
                        if (v < 0) { putc_all('-'); v = -v; }
                        print_num((uint32_t)v, 10, width, pad); break; }
            case 'u': print_num(va_arg(ap, uint32_t), 10, width, pad); break;
            case 'x': print_num(va_arg(ap, uint32_t), 16, width, pad); break;
            case 'p': putc_all('0'); putc_all('x'); print_num(va_arg(ap, uint32_t), 16, 8, '0'); break;
            case '%': putc_all('%'); break;
            default:  putc_all('%'); putc_all(*fmt); break;
        }
    }
    va_end(ap);
}

void panic(const char *msg) {
    cli();
    kprintf("\nKERNEL PANIC: %s\nSystem halted.\n", msg);
    for (;;) hlt();
}

#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ---------- port I/O ---------- */
static inline void outb(uint16_t port, uint8_t v) { __asm__ volatile("outb %0, %1" : : "a"(v), "Nd"(port)); }
static inline uint8_t inb(uint16_t port) { uint8_t v; __asm__ volatile("inb %1, %0" : "=a"(v) : "Nd"(port)); return v; }
static inline void io_wait(void) { outb(0x80, 0); }
static inline void hlt(void) { __asm__ volatile("hlt"); }
static inline void sti(void) { __asm__ volatile("sti"); }
static inline void cli(void) { __asm__ volatile("cli"); }

/* ---------- interrupt frame (pushed by isr.asm) ---------- */
typedef struct regs {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags;
} regs_t;

typedef void (*irq_handler_t)(regs_t *);

/* ---------- lib ---------- */
void  *memset(void *d, int c, size_t n);
void  *memcpy(void *d, const void *s, size_t n);
size_t strlen(const char *s);
int    strcmp(const char *a, const char *b);
int    strncmp(const char *a, const char *b, size_t n);
int    atoi(const char *s);
void   kprintf(const char *fmt, ...);
void   panic(const char *msg);

/* ---------- arch ---------- */
void gdt_init(void);
void idt_init(void);
void irq_register(int irq, irq_handler_t h);

/* ---------- drivers ---------- */
void vga_init(void);
void vga_putc(char c);
void vga_clear(void);
void vga_set_color(uint8_t fg, uint8_t bg);
void serial_init(void);
void serial_putc(char c);
int  serial_getc_nonblock(void);
void timer_init(uint32_t hz);
uint32_t timer_ticks(void);
void timer_sleep(uint32_t ms);
void keyboard_init(void);
int  keyboard_getc_nonblock(void);

/* ---------- memory ---------- */
void     pmm_init(uint32_t mb_info_addr);
uint32_t pmm_alloc_frame(void);
void     pmm_free_frame(uint32_t addr);
uint32_t pmm_free_frames(void);
uint32_t pmm_total_frames(void);
void     kheap_init(void);
void    *kmalloc(size_t n);
void     kfree(void *p);
void     kheap_stats(size_t *used, size_t *free_bytes, int *blocks);

void paging_init(void);
bool paging_translate(uint32_t virt, uint32_t *phys);

/* ---------- shell ---------- */
void shell_run(void);

#endif

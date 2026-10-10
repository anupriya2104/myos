/* Global Descriptor Table: flat 4 GiB code/data segments for ring 0 and ring 3.
 * Selectors: 0x08 kernel code, 0x10 kernel data, 0x18 user code, 0x20 user data. */
#include "kernel.h"

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  gran;      /* high nibble: flags, low nibble: limit bits 16..19 */
    uint8_t  base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct gdt_entry gdt[5];
static struct gdt_ptr gp;

extern void gdt_flush(uint32_t ptr);

static void set(int i, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    gdt[i].base_low  = base & 0xFFFF;
    gdt[i].base_mid  = (base >> 16) & 0xFF;
    gdt[i].base_high = (base >> 24) & 0xFF;
    gdt[i].limit_low = limit & 0xFFFF;
    gdt[i].gran      = ((limit >> 16) & 0x0F) | (flags & 0xF0);
    gdt[i].access    = access;
}

void gdt_init(void) {
    gp.limit = sizeof(gdt) - 1;
    gp.base  = (uint32_t)&gdt;
    set(0, 0, 0, 0, 0);                    /* null descriptor */
    set(1, 0, 0xFFFFF, 0x9A, 0xC0);        /* kernel code: present, ring0, exec/read */
    set(2, 0, 0xFFFFF, 0x92, 0xC0);        /* kernel data: present, ring0, read/write */
    set(3, 0, 0xFFFFF, 0xFA, 0xC0);        /* user code:   ring3 */
    set(4, 0, 0xFFFFF, 0xF2, 0xC0);        /* user data:   ring3 */
    gdt_flush((uint32_t)&gp);
}

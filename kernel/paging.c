/* x86 32-bit paging: two-level page tables (page directory -> page tables
 * -> 4 KiB frames). We identity-map the first 64 MiB so virtual == physical
 * for the kernel, then turn on CR0.PG. Anything outside that range is
 * unmapped, so touching it raises a page fault (#PF, vector 14). */
#include "kernel.h"

#define PRESENT  0x1
#define WRITABLE 0x2
#define MAPPED_MB 64

static uint32_t page_dir[1024] __attribute__((aligned(4096)));

void paging_init(void) {
    memset(page_dir, 0, sizeof(page_dir));
    for (uint32_t t = 0; t < MAPPED_MB / 4; t++) {           /* each table maps 4 MiB */
        uint32_t *table = (uint32_t *)pmm_alloc_frame();
        if (!table) panic("paging: out of frames");
        for (uint32_t i = 0; i < 1024; i++)
            table[i] = ((t * 1024 + i) * 4096) | PRESENT | WRITABLE;
        page_dir[t] = (uint32_t)table | PRESENT | WRITABLE;
    }
    __asm__ volatile("mov %0, %%cr3" : : "r"(page_dir));      /* load page directory */
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;                                       /* CR0.PG = 1 */
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
}

/* Walks the page tables in software, the same way the MMU does in hardware. */
bool paging_translate(uint32_t virt, uint32_t *phys) {
    uint32_t pde = page_dir[virt >> 22];
    if (!(pde & PRESENT)) return false;
    uint32_t pte = ((uint32_t *)(pde & ~0xFFF))[(virt >> 12) & 0x3FF];
    if (!(pte & PRESENT)) return false;
    *phys = (pte & ~0xFFF) | (virt & 0xFFF);
    return true;
}

/* Physical memory manager: a bitmap with one bit per 4 KiB frame.
 * Built from the multiboot memory map (BIOS E820 data), so only RAM the
 * firmware reports as usable is ever handed out. */
#include "kernel.h"

#define FRAME 4096
#define MAX_FRAMES (1u << 20)                /* 4 GiB / 4 KiB */

struct mb_info {
    uint32_t flags, mem_lower, mem_upper, boot_device, cmdline;
    uint32_t mods_count, mods_addr, syms[4];
    uint32_t mmap_length, mmap_addr;
} __attribute__((packed));

struct mb_mmap {
    uint32_t size;                           /* size of the rest of this entry */
    uint64_t addr, len;
    uint32_t type;                           /* 1 = usable RAM */
} __attribute__((packed));

extern char kernel_end[];                    /* from linker.ld */
extern char heap_start[], heap_end[];

static uint32_t bitmap[MAX_FRAMES / 32];     /* bit set = frame used */
static uint32_t total, used, max_frame;
static uint32_t search_from;                  /* next-fit hint */

static inline void set_used(uint32_t f) { bitmap[f / 32] |= 1u << (f % 32); }
static inline void set_free(uint32_t f) { bitmap[f / 32] &= ~(1u << (f % 32)); }
static inline bool is_used(uint32_t f) { return bitmap[f / 32] & (1u << (f % 32)); }

static void reserve(uint32_t start, uint32_t end) {
    for (uint32_t f = start / FRAME; f < (end + FRAME - 1) / FRAME && f < max_frame; f++)
        if (!is_used(f)) { set_used(f); used++; }
}

void pmm_init(uint32_t mb_addr) {
    struct mb_info *mb = (struct mb_info *)mb_addr;
    memset(bitmap, 0xFF, sizeof(bitmap));    /* start with everything reserved */

    if (!(mb->flags & (1 << 6))) panic("bootloader gave no memory map");

    kprintf("Memory map (from BIOS E820 via multiboot):\n");
    uint32_t p = mb->mmap_addr;
    while (p < mb->mmap_addr + mb->mmap_length) {
        struct mb_mmap *e = (struct mb_mmap *)p;
        kprintf("  0x%08x - 0x%08x  %s\n", (uint32_t)e->addr,
                (uint32_t)(e->addr + e->len - 1), e->type == 1 ? "usable" : "reserved");
        if (e->type == 1 && e->addr < 0x100000000ULL) {
            uint64_t end = e->addr + e->len;
            if (end > 0x100000000ULL) end = 0x100000000ULL;
            uint32_t first = (e->addr + FRAME - 1) / FRAME;   /* round start up */
            uint32_t last = end / FRAME;                      /* round end down */
            for (uint32_t f = first; f < last; f++) { set_free(f); total++; }
            if (last > max_frame) max_frame = last;
        }
        p += e->size + sizeof(e->size);
    }
    used = 0;
    reserve(0, 0x100000);                                   /* BIOS, VGA, low memory */
    reserve(0x100000, (uint32_t)kernel_end);                /* kernel image + bss */
    reserve((uint32_t)heap_start, (uint32_t)heap_end);      /* kernel heap */
    search_from = 0x100000 / FRAME;
}

uint32_t pmm_alloc_frame(void) {
    for (uint32_t i = 0; i < max_frame; i++) {
        uint32_t f = (search_from + i) % max_frame;
        if (bitmap[f / 32] == 0xFFFFFFFF) { i += 31 - (f % 32); continue; }  /* skip full words */
        if (!is_used(f)) {
            set_used(f); used++;
            search_from = f + 1;
            return f * FRAME;
        }
    }
    return 0;                                                /* out of memory */
}

void pmm_free_frame(uint32_t addr) {
    uint32_t f = addr / FRAME;
    if (f >= max_frame || !is_used(f)) panic("pmm: double free or bad address");
    set_free(f); used--;
}

uint32_t pmm_total_frames(void) { return total; }
uint32_t pmm_free_frames(void) { return total - used; }

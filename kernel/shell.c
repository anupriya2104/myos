/* Interactive kernel shell. Reads from the PS/2 keyboard and the serial
 * port, so it works both in a QEMU window and headless over -serial stdio. */
#include "kernel.h"

#define LINE 128

static int getc_blocking(void) {
    for (;;) {
        int c = keyboard_getc_nonblock();
        if (c < 0) c = serial_getc_nonblock();
        if (c >= 0) return c == '\r' ? '\n' : c;
        hlt();                              /* wait for the next interrupt (timer/keyboard) */
    }
}

static void readline(char *buf) {
    int n = 0;
    for (;;) {
        int c = getc_blocking();
        if (c == '\n') { kprintf("\n"); buf[n] = 0; return; }
        if (c == '\b' || c == 127) {
            if (n > 0) { n--; kprintf("\b \b"); }
            continue;
        }
        if (c >= 32 && n < LINE - 1) { buf[n++] = c; kprintf("%c", c); }
    }
}

static uint32_t parse_hex(const char *s) {
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
    uint32_t v = 0;
    for (; *s; s++) {
        char c = *s;
        if (c >= '0' && c <= '9') v = v * 16 + c - '0';
        else if (c >= 'a' && c <= 'f') v = v * 16 + c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') v = v * 16 + c - 'A' + 10;
        else break;
    }
    return v;
}

static void cmd_cpuid(void) {
    uint32_t a, b, c, d;
    char vendor[13];
    __asm__ volatile("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(0));
    memcpy(vendor, &b, 4); memcpy(vendor + 4, &d, 4); memcpy(vendor + 8, &c, 4);
    vendor[12] = 0;
    kprintf("Vendor: %s (max leaf %u)\n", vendor, a);
    __asm__ volatile("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(1));
    kprintf("Family %u, model %u, stepping %u\n", (a >> 8) & 0xF, (a >> 4) & 0xF, a & 0xF);
    kprintf("Features:");
    if (d & (1 << 0))  kprintf(" fpu");
    if (d & (1 << 4))  kprintf(" tsc");
    if (d & (1 << 6))  kprintf(" pae");
    if (d & (1 << 9))  kprintf(" apic");
    if (d & (1 << 23)) kprintf(" mmx");
    if (d & (1 << 25)) kprintf(" sse");
    if (d & (1 << 26)) kprintf(" sse2");
    if (c & (1 << 0))  kprintf(" sse3");
    if (c & (1 << 19)) kprintf(" sse4.1");
    if (c & (1 << 20)) kprintf(" sse4.2");
    if (c & (1 << 28)) kprintf(" avx");
    kprintf("\n");
}

static void cmd_mem(void) {
    uint32_t total = pmm_total_frames(), free = pmm_free_frames();
    kprintf("Physical: %u KiB total, %u KiB free (%u / %u frames of 4 KiB)\n",
            total * 4, free * 4, free, total);
    size_t used, hfree; int blocks;
    kheap_stats(&used, &hfree, &blocks);
    kprintf("Heap:     %u bytes used, %u bytes free, %d blocks\n",
            (uint32_t)used, (uint32_t)hfree, blocks);
}

static void cmd_heaptest(void) {
    size_t u0, f0; int b0;
    kheap_stats(&u0, &f0, &b0);
    void *p[8];
    for (int i = 0; i < 8; i++) p[i] = kmalloc(64 * (i + 1));
    kprintf("Allocated 8 blocks: first at %p, last at %p\n", (uint32_t)p[0], (uint32_t)p[7]);
    for (int i = 0; i < 8; i += 2) kfree(p[i]);         /* free every other one: fragment */
    size_t u, f; int b;
    kheap_stats(&u, &f, &b);
    kprintf("After freeing 4: %d blocks (fragmented)\n", b);
    for (int i = 1; i < 8; i += 2) kfree(p[i]);
    kheap_stats(&u, &f, &b);
    kprintf("After freeing all: %d block(s), %u bytes free -> %s\n", b, (uint32_t)f,
            (b == b0 && f == f0) ? "coalesced OK" : "LEAK");
}

static void cmd_help(void) {
    kprintf("Commands:\n"
            "  help              this list\n"
            "  clear             clear the screen\n"
            "  echo <text>       print text\n"
            "  uptime            ticks and seconds since boot (PIT @ 100 Hz)\n"
            "  sleep <ms>        sleep using timer interrupts\n"
            "  mem               physical frames and heap usage\n"
            "  heaptest          allocate/free and check the heap coalesces\n"
            "  alloc             allocate a physical frame\n"
            "  translate <hex>   walk the page tables for a virtual address\n"
            "  cpuid             CPU vendor and feature flags\n"
            "  color <fg> <bg>   set text color (0-15)\n"
            "  pagefault         read an unmapped address (triggers #PF)\n"
            "  divzero           divide by zero (triggers #DE)\n"
            "  reboot            reset via the keyboard controller\n");
}

void shell_run(void) {
    char line[LINE];
    for (;;) {
        kprintf("myos> ");
        readline(line);
        char *arg = line;
        while (*arg && *arg != ' ') arg++;
        if (*arg) *arg++ = 0;

        if (!line[0]) continue;
        else if (!strcmp(line, "help")) cmd_help();
        else if (!strcmp(line, "clear")) vga_clear();
        else if (!strcmp(line, "echo")) kprintf("%s\n", arg);
        else if (!strcmp(line, "uptime")) {
            uint32_t t = timer_ticks();
            kprintf("%u ticks, %u.%02u s\n", t, t / 100, t % 100);
        }
        else if (!strcmp(line, "sleep")) {
            uint32_t t0 = timer_ticks();
            timer_sleep(atoi(arg));
            kprintf("slept %u ticks\n", timer_ticks() - t0);
        }
        else if (!strcmp(line, "mem")) cmd_mem();
        else if (!strcmp(line, "heaptest")) cmd_heaptest();
        else if (!strcmp(line, "alloc")) kprintf("frame at %p\n", pmm_alloc_frame());
        else if (!strcmp(line, "translate")) {
            uint32_t v = parse_hex(arg), p;
            if (paging_translate(v, &p))
                kprintf("virt %p -> phys %p (dir %u, table %u, offset 0x%x)\n",
                        v, p, v >> 22, (v >> 12) & 0x3FF, v & 0xFFF);
            else kprintf("virt %p is not mapped\n", v);
        }
        else if (!strcmp(line, "cpuid")) cmd_cpuid();
        else if (!strcmp(line, "color")) {
            char *bg = arg;
            while (*bg && *bg != ' ') bg++;
            vga_set_color(atoi(arg), *bg ? atoi(bg + 1) : 0);
        }
        else if (!strcmp(line, "pagefault")) {
            volatile uint32_t *bad = (uint32_t *)0xDEADB000;
            kprintf("reading %p ...\n", (uint32_t)bad);
            kprintf("%u\n", *bad);
        }
        else if (!strcmp(line, "divzero")) {
            /* GCC rewrites 1/z as a compare (no div instruction), so issue div directly */
            uint32_t q, z = 0;
            __asm__ volatile("xor %%edx, %%edx\n\tdiv %1" : "=a"(q) : "r"(z), "a"(100) : "edx");
            kprintf("%u\n", q);
        }
        else if (!strcmp(line, "reboot")) {
            while (inb(0x64) & 0x02) { }
            outb(0x64, 0xFE);                 /* pulse the CPU reset line */
        }
        else kprintf("unknown command: %s (try 'help')\n", line);
    }
}

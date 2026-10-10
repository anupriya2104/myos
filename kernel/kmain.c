/* Kernel entry: called from boot/boot.asm with the multiboot magic and
 * info pointer. Brings subsystems up in dependency order. */
#include "kernel.h"

#define MULTIBOOT_MAGIC 0x2BADB002

void kmain(uint32_t magic, uint32_t mb_info) {
    vga_init();
    serial_init();
    vga_set_color(11, 0);
    kprintf("MyOS 0.1 - 32-bit x86 kernel in C\n\n");
    vga_set_color(7, 0);

    if (magic != MULTIBOOT_MAGIC) panic("not loaded by a multiboot bootloader");

    gdt_init();      kprintf("[ok] GDT: flat segments, kernel 0x08/0x10, user 0x18/0x20\n");
    idt_init();      kprintf("[ok] IDT: 32 exception + 16 IRQ gates, PIC remapped to 32-47\n");
    pmm_init(mb_info);
    kprintf("[ok] PMM: %u KiB usable RAM, bitmap allocator\n", pmm_total_frames() * 4);
    paging_init();   kprintf("[ok] Paging: identity-mapped first 64 MiB, CR0.PG on\n");
    kheap_init();    kprintf("[ok] Heap: first-fit allocator with coalescing\n");
    timer_init(100); kprintf("[ok] PIT: 100 Hz\n");
    keyboard_init(); kprintf("[ok] PS/2 keyboard on IRQ1\n");
    sti();           kprintf("[ok] Interrupts enabled\n\n");

    kprintf("Type 'help' for commands.\n");
    shell_run();
}

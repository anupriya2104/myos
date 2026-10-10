/* Kernel memory layout. The bootloader loads the ELF at 1 MiB (below that
 * is BIOS data, VGA memory and ROM). The multiboot header must sit in the
 * first 8 KiB of the file, so .multiboot goes first. */
ENTRY(_start)

SECTIONS
{
    . = 1M;
    kernel_start = .;

    .text ALIGN(4K) : {
        *(.multiboot)
        *(.text*)
    }
    .rodata ALIGN(4K) : { *(.rodata*) }
    .data   ALIGN(4K) : { *(.data*) }
    .bss    ALIGN(4K) : {
        *(COMMON)
        *(.bss*)
    }

    /* 1 MiB kernel heap, reserved right after the image */
    . = ALIGN(4K);
    heap_start = .;
    . += 1M;
    heap_end = .;

    kernel_end = .;
}

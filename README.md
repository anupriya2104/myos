# MyOS

A small 32-bit x86 operating system kernel written from scratch in **C and x86 assembly**. It boots through Multiboot (GRUB or QEMU), sets up the CPU's segmentation, interrupts and paging, manages physical memory from the BIOS memory map, and drops into an interactive shell.

```
MyOS 0.1 - 32-bit x86 kernel in C

[ok] GDT: flat segments, kernel 0x08/0x10, user 0x18/0x20
[ok] IDT: 32 exception + 16 IRQ gates, PIC remapped to 32-47
Memory map (from BIOS E820 via multiboot):
  0x00000000 - 0x0009fbff  usable
  0x00100000 - 0x07fdffff  usable
  ...
[ok] PMM: 130556 KiB usable RAM, bitmap allocator
[ok] Paging: identity-mapped first 64 MiB, CR0.PG on
[ok] Heap: first-fit allocator with coalescing
[ok] PIT: 100 Hz
[ok] PS/2 keyboard on IRQ1
[ok] Interrupts enabled

myos> translate 0xb8000
virt 0x000b8000 -> phys 0x000b8000 (dir 0, table 184, offset 0x0)
myos> pagefault
reading 0xdeadb000 ...
*** CPU EXCEPTION 14: Page fault ***
err=0x0 eip=0x1013b2 cs=0x8 eflags=0x216 cr2=0xdeadb000
```

## Build and run

Needs `gcc` (with 32-bit support), `nasm`, `ld` and `qemu-system-i386`.
On Ubuntu: `sudo apt install build-essential gcc-multilib nasm qemu-system-x86`.

```
make                 # build build/myos.elf
make run             # boot in a QEMU window (shell also mirrored to the terminal)
make run-headless    # boot with no window; use the shell over the terminal
make test            # boot headless, type commands over serial, check output
```

## Project structure

```
boot/boot.asm        Multiboot header, stack setup, jump into C
arch/gdt.c           Global Descriptor Table (ring 0 / ring 3 segments)
arch/idt.c           Interrupt Descriptor Table, 8259 PIC remap, dispatch, exception reports
arch/isr.asm         48 interrupt entry stubs, GDT/IDT load, register save/restore
kernel/kmain.c       Boot sequence
kernel/pmm.c         Physical memory manager (bitmap over 4 KiB frames, from E820 map)
kernel/paging.c      Two-level page tables, CR3/CR0 setup, software page walk
kernel/kheap.c       kmalloc/kfree: first-fit, block splitting, coalescing
kernel/shell.c       Interactive shell
drivers/vga.c        VGA text mode (0xB8000), scrolling, hardware cursor
drivers/serial.c     16550 UART on COM1 (output mirror + input)
drivers/timer.c      8254 PIT on IRQ0
drivers/keyboard.c   PS/2 keyboard on IRQ1, scancode set 1 -> ASCII, ring buffer
lib/string.c         Freestanding memset/strcmp/kprintf/panic (no libc in the kernel)
linker.ld            Kernel memory layout (load at 1 MiB, heap region, symbols)
tests/run_tests.sh   Automated boot tests in QEMU
```

## How it works

**1. Boot.** The bootloader loads the kernel ELF at 1 MiB in 32-bit protected mode with paging off, and passes a magic number in `EAX` and a pointer to the Multiboot info in `EBX`. `boot.asm` sets up a 16 KiB stack and calls `kmain(magic, info)`. The Multiboot header has to be within the first 8 KiB of the file, which is why the linker script puts `.multiboot` first.

**2. Segmentation (GDT).** x86 still requires segment descriptors in protected mode. MyOS uses a flat model: every segment has base 0 and limit 4 GiB, so segmentation does nothing except set privilege level. After `lgdt`, a far jump (`jmp 0x08:...`) is needed to reload `CS`, because `CS` can't be loaded with `mov`.

**3. Interrupts (IDT + PIC).** Vectors 0-31 are CPU exceptions. The legacy BIOS maps hardware IRQs 0-7 to vectors 8-15, which collide with exceptions (IRQ0 would look like a double fault), so the 8259 PIC is reprogrammed to deliver IRQs at 32-47. Some exceptions push an error code and some don't; the stubs push a dummy 0 where needed so every handler sees the same `regs_t` frame. Each IRQ handler ends with an End-of-Interrupt to the PIC, or no further interrupts arrive.

**4. Physical memory.** The BIOS E820 memory map (passed via Multiboot) says which ranges are RAM. A bitmap tracks one bit per 4 KiB frame. Everything starts reserved; usable regions are freed; then low memory, the kernel image and the heap are re-reserved using symbols exported by the linker script (`kernel_end`, `heap_start`).

**5. Paging.** A page directory (1024 entries) points to page tables (1024 entries each), each entry mapping a 4 KiB frame, so one table covers 4 MiB. The kernel identity-maps the first 64 MiB, loads `CR3` and sets `CR0.PG`. A virtual address splits into directory index (bits 31-22), table index (21-12) and offset (11-0); the `translate` command does this walk in software. Accessing an unmapped address raises a page fault, and the faulting address is read from `CR2`.

**6. Heap.** `kmalloc` walks a linked list of blocks (first fit), splits large free blocks, and returns 16-byte-aligned memory. `kfree` merges adjacent free blocks; `heaptest` checks that freeing everything restores a single block.

**7. Drivers.** The PIT is programmed with divisor 1193182 / 100 for a 100 Hz tick, and `sleep` halts the CPU until enough ticks have passed. The keyboard translates scancodes, tracking shift and caps lock. All output also goes to the serial port, which is how the tests run without a screen.

## Shell commands

`help`, `clear`, `echo`, `uptime`, `sleep <ms>`, `date`, `mem`, `heaptest`, `alloc`, `translate <hex>`, `cpuid`, `color <fg> <bg>`, `pagefault`, `divzero`, `reboot`.

## Next steps

- Higher-half kernel and per-process address spaces
- Task switching with a TSS and a round-robin scheduler on the PIT tick
- Ring 3 user programs and a system-call interface (`int 0x80`)
- An ELF loader for user programs
- A simple filesystem (FAT12/initrd)

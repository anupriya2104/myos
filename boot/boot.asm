; Multiboot (v1) entry point. GRUB or QEMU's -kernel loads us in 32-bit
; protected mode with paging off, EAX = magic, EBX = multiboot info pointer.

MB_ALIGN    equ 1 << 0              ; align modules on page boundaries
MB_MEMINFO  equ 1 << 1              ; ask the loader for a memory map
MB_FLAGS    equ MB_ALIGN | MB_MEMINFO
MB_MAGIC    equ 0x1BADB002
MB_CHECKSUM equ -(MB_MAGIC + MB_FLAGS)

section .multiboot
align 4
    dd MB_MAGIC
    dd MB_FLAGS
    dd MB_CHECKSUM

section .bss
align 16
stack_bottom:
    resb 16384                      ; 16 KiB kernel stack
stack_top:

section .text
global _start
extern kmain
_start:
    mov esp, stack_top              ; set up the stack before any C code
    push ebx                        ; arg 2: multiboot info address
    push eax                        ; arg 1: multiboot magic
    call kmain
.hang:
    cli
    hlt
    jmp .hang

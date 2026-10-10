; Low-level CPU glue: GDT reload, and the 48 interrupt entry stubs
; (32 CPU exceptions + 16 hardware IRQs) that build a regs_t frame
; and call into C.

section .text

global gdt_flush
gdt_flush:
    mov eax, [esp + 4]
    lgdt [eax]
    mov ax, 0x10                    ; kernel data selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.reload_cs             ; far jump reloads CS with kernel code selector
.reload_cs:
    ret

global idt_load
idt_load:
    mov eax, [esp + 4]
    lidt [eax]
    ret

; Exceptions 8, 10-14, 17, 21 push an error code; the rest don't, so we push
; a dummy 0 to keep the stack frame identical for every vector.
%macro ISR_NOERR 1
global isr%1
isr%1:
    push dword 0
    push dword %1
    jmp common_stub
%endmacro

%macro ISR_ERR 1
global isr%1
isr%1:
    push dword %1
    jmp common_stub
%endmacro

%macro IRQ 2
global irq%1
irq%1:
    push dword 0
    push dword %2
    jmp common_stub
%endmacro

ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_ERR   21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

extern interrupt_dispatch
common_stub:
    pusha                           ; eax, ecx, edx, ebx, esp, ebp, esi, edi
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10                    ; run the handler with kernel data segments
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    push esp                        ; arg: pointer to regs_t on the stack
    call interrupt_dispatch
    add esp, 4
    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8                      ; drop int_no and err_code
    iret

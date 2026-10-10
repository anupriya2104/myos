/* Interrupt Descriptor Table + 8259 PIC setup and dispatch.
 * Vectors 0-31: CPU exceptions. Vectors 32-47: hardware IRQs (PIC remapped). */
#include "kernel.h"

struct idt_entry {
    uint16_t base_low;
    uint16_t sel;
    uint8_t  zero;
    uint8_t  flags;
    uint16_t base_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_ptr ip;
static irq_handler_t irq_handlers[16];

extern void idt_load(uint32_t);
#define DECL(n) extern void isr##n(void);
#define DECLI(n) extern void irq##n(void);
DECL(0) DECL(1) DECL(2) DECL(3) DECL(4) DECL(5) DECL(6) DECL(7)
DECL(8) DECL(9) DECL(10) DECL(11) DECL(12) DECL(13) DECL(14) DECL(15)
DECL(16) DECL(17) DECL(18) DECL(19) DECL(20) DECL(21) DECL(22) DECL(23)
DECL(24) DECL(25) DECL(26) DECL(27) DECL(28) DECL(29) DECL(30) DECL(31)
DECLI(0) DECLI(1) DECLI(2) DECLI(3) DECLI(4) DECLI(5) DECLI(6) DECLI(7)
DECLI(8) DECLI(9) DECLI(10) DECLI(11) DECLI(12) DECLI(13) DECLI(14) DECLI(15)

static void (*const stubs[48])(void) = {
    isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7, isr8, isr9, isr10, isr11,
    isr12, isr13, isr14, isr15, isr16, isr17, isr18, isr19, isr20, isr21, isr22,
    isr23, isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31,
    irq0, irq1, irq2, irq3, irq4, irq5, irq6, irq7, irq8, irq9, irq10, irq11,
    irq12, irq13, irq14, irq15,
};

static const char *exc_names[32] = {
    "Divide by zero", "Debug", "NMI", "Breakpoint", "Overflow", "Bound range",
    "Invalid opcode", "Device not available", "Double fault", "Coprocessor overrun",
    "Invalid TSS", "Segment not present", "Stack fault", "General protection fault",
    "Page fault", "Reserved", "x87 FPU error", "Alignment check", "Machine check",
    "SIMD FP exception", "Virtualization", "Control protection", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved", "Hypervisor injection",
    "VMM communication", "Security", "Reserved",
};

static void set_gate(int n, uint32_t base) {
    idt[n].base_low  = base & 0xFFFF;
    idt[n].base_high = (base >> 16) & 0xFFFF;
    idt[n].sel       = 0x08;      /* kernel code segment */
    idt[n].zero      = 0;
    idt[n].flags     = 0x8E;      /* present, ring 0, 32-bit interrupt gate */
}

/* The BIOS maps IRQ 0-7 to vectors 8-15, which collide with CPU exceptions.
 * Remap master PIC to 32-39 and slave PIC to 40-47. */
static void pic_remap(void) {
    outb(0x20, 0x11); io_wait();  /* ICW1: init, expect ICW4 */
    outb(0xA0, 0x11); io_wait();
    outb(0x21, 0x20); io_wait();  /* ICW2: master vector offset 32 */
    outb(0xA1, 0x28); io_wait();  /* ICW2: slave vector offset 40 */
    outb(0x21, 0x04); io_wait();  /* ICW3: slave on IRQ2 */
    outb(0xA1, 0x02); io_wait();
    outb(0x21, 0x01); io_wait();  /* ICW4: 8086 mode */
    outb(0xA1, 0x01); io_wait();
    outb(0x21, 0xFF);             /* mask everything; drivers unmask their IRQ */
    outb(0xA1, 0xFF);
}

void irq_register(int irq, irq_handler_t h) {
    irq_handlers[irq] = h;
    uint16_t port = irq < 8 ? 0x21 : 0xA1;
    uint8_t bit = irq < 8 ? irq : irq - 8;
    outb(port, inb(port) & ~(1 << bit));
    if (irq >= 8) outb(0x21, inb(0x21) & ~(1 << 2));   /* unmask cascade line */
}

void interrupt_dispatch(regs_t *r) {
    if (r->int_no < 32) {
        uint32_t cr2;
        __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
        vga_set_color(15, 4);
        kprintf("\n*** CPU EXCEPTION %u: %s ***\n", r->int_no, exc_names[r->int_no]);
        kprintf("err=0x%x eip=0x%x cs=0x%x eflags=0x%x", r->err_code, r->eip, r->cs, r->eflags);
        if (r->int_no == 14) kprintf(" cr2=0x%x", cr2);
        kprintf("\neax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n", r->eax, r->ebx, r->ecx, r->edx);
        panic("unhandled exception");
    }
    int irq = r->int_no - 32;
    if (irq_handlers[irq]) irq_handlers[irq](r);
    if (irq >= 8) outb(0xA0, 0x20);   /* EOI to slave */
    outb(0x20, 0x20);                 /* EOI to master */
}

void idt_init(void) {
    ip.limit = sizeof(idt) - 1;
    ip.base  = (uint32_t)&idt;
    memset(idt, 0, sizeof(idt));
    pic_remap();
    for (int i = 0; i < 48; i++) set_gate(i, (uint32_t)stubs[i]);
    idt_load((uint32_t)&ip);
}

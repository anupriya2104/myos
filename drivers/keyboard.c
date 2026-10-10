/* PS/2 keyboard on IRQ1: translates scancode set 1 to ASCII and stores
 * characters in a ring buffer the shell reads from. */
#include "kernel.h"

#define BUF 128
static volatile char buf[BUF];
static volatile uint32_t head, tail;
static bool shift, caps;

static const char normal[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0, 'a','s','d','f','g','h','j','k','l',';','\'','`',
    0, '\\','z','x','c','v','b','n','m',',','.','/', 0,
    '*', 0, ' ',
};

static const char shifted[128] = {
    0, 27, '!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0, 'A','S','D','F','G','H','J','K','L',':','"','~',
    0, '|','Z','X','C','V','B','N','M','<','>','?', 0,
    '*', 0, ' ',
};

static void push(char c) {
    uint32_t next = (head + 1) % BUF;
    if (next != tail) { buf[head] = c; head = next; }   /* drop if full */
}

static void on_key(regs_t *r) {
    (void)r;
    uint8_t sc = inb(0x60);
    bool release = sc & 0x80;
    sc &= 0x7F;
    if (sc == 0x2A || sc == 0x36) { shift = !release; return; }   /* L/R shift */
    if (release) return;
    if (sc == 0x3A) { caps = !caps; return; }                       /* caps lock */
    char c = shift ? shifted[sc] : normal[sc];
    if (caps && c >= 'a' && c <= 'z') c -= 32;
    else if (caps && c >= 'A' && c <= 'Z') c += 32;
    if (c) push(c);
}

void keyboard_init(void) { irq_register(1, on_key); }

int keyboard_getc_nonblock(void) {
    if (head == tail) return -1;
    char c = buf[tail];
    tail = (tail + 1) % BUF;
    return (uint8_t)c;
}

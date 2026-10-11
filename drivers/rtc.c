/* CMOS real-time clock. The RTC keeps the date and time on battery power.
 * Its registers are read by writing a register number to port 0x70 and
 * reading the value from port 0x71. */
#include "kernel.h"

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

static uint8_t cmos_read(uint8_t reg) {
    outb(CMOS_ADDR, reg);
    return inb(CMOS_DATA);
}

/* Register A bit 7 is set while the RTC is updating its registers;
 * reading during an update can give a half-changed value. */
static bool updating(void) { return cmos_read(0x0A) & 0x80; }

static uint8_t bcd_to_bin(uint8_t v) { return (v & 0x0F) + (v >> 4) * 10; }

static void read_raw(rtc_time_t *t) {
    while (updating()) { }
    t->sec   = cmos_read(0x00);
    t->min   = cmos_read(0x02);
    t->hour  = cmos_read(0x04);
    t->day   = cmos_read(0x07);
    t->month = cmos_read(0x08);
    t->year  = cmos_read(0x09);
}

void rtc_read(rtc_time_t *t) {
    /* Read twice until two reads agree, so an update between
     * individual register reads can't give an inconsistent time. */
    rtc_time_t a, b;
    read_raw(&a);
    do {
        b = a;
        read_raw(&a);
    } while (a.sec != b.sec || a.min != b.min || a.hour != b.hour ||
             a.day != b.day || a.month != b.month || a.year != b.year);

    uint8_t regb = cmos_read(0x0B);
    bool pm = a.hour & 0x80;                 /* 12-hour mode PM flag */
    a.hour &= 0x7F;

    if (!(regb & 0x04)) {                    /* bit 2 clear: values are BCD */
        a.sec   = bcd_to_bin(a.sec);
        a.min   = bcd_to_bin(a.min);
        a.hour  = bcd_to_bin(a.hour);
        a.day   = bcd_to_bin(a.day);
        a.month = bcd_to_bin(a.month);
        a.year  = bcd_to_bin(a.year);
    }
    if (!(regb & 0x02) && pm)                /* bit 1 clear: 12-hour mode */
        a.hour = (a.hour + 12) % 24;

    a.year += 2000;                          /* RTC stores only two digits */
    *t = a;
}

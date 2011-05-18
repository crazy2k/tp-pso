#include <i386.h>
#include <tipos.h>
#include <stdarg.h>
#include "vga.h"

uint_16 vga_port = 0x3D0;

void vga_init(void) {
}

void *vga_putchar(void *addr, const char chr, uint8_t attr) {
    uint8_t *bytepos = (uint8_t *)addr;
    bytepos[0] = chr;
    bytepos[1] = attr;

    return addr + VGA_CHAR_SIZE;
}

void *vga_puts(void *addr, const char *msg, uint8_t attr) {
    char c;
    while ((c = *msg++) != '\0') {
        vga_putchar(addr, c, attr);
        addr += VGA_CHAR_SIZE;
    }
    return addr;
}


void vga_write(uint_16 row, uint_16 col, const char *msg, uint_8 attr) {
    void *addr = VGA_POS_ADDR(row, col);
    vga_puts(addr, msg, attr);
}




void vga_writechar(uint16_t row, uint16_t col, const char chr, uint8_t attr) {
    vga_putchar(VGA_POS_ADDR(row, col), chr, attr);
}


#define VGA_TEMPBUFSIZE 40
void *vga_putd(void *addr, long int num, uint8_t attr) {
    if (num < 0) {
        vga_putchar(addr, '-', attr);
        num = -num;
    }

    char chars[VGA_TEMPBUFSIZE] = {'\0'};
    int i = VGA_TEMPBUFSIZE - 1;
    do {
        chars[--i] = (char)(num % 10ul) + '0';
        num /= 10ul;
    } while(num != 0);

    return vga_puts(addr, &chars[i], attr);
}


#define VGA_UI_BASE 16
#define VGA_UI_LOG2BASE 4
#define VGA_UI_REPLENGTH (sizeof(uint32_t)*8/VGA_UI_LOG2BASE)
void *vga_putx(void *addr, uint32_t n, uint8_t attr) {
    char chars[VGA_UI_BASE],
         str[VGA_UI_REPLENGTH + 1],
         base = '0';

    int i;

    for (i = 0; i < VGA_UI_BASE; i++) {
        if (i == 10)
            base = 'A';

        chars[i] = base + (i % 10);
    }

    for (i = VGA_UI_REPLENGTH - 1; i >= 0; i--, n /= VGA_UI_BASE)
        str[i] = chars[n % VGA_UI_BASE];

    str[VGA_UI_REPLENGTH] = '\0';

    addr = vga_puts(addr, "0x", attr);
    return vga_puts(addr, str, attr);
}


void vga_printf(uint_16 row, uint_16 col, const char* format, uint_8 attr,
    ...) {
    va_list vargs;
    va_start(vargs, attr);

    char c;
    void *addr = VGA_POS_ADDR(row, col);
    while ((c = *format++) != '\0') {
        if (c != '%')
            addr = vga_putchar(addr, c, attr);
        else {
            c = *format++;
            if (c == 'd')
                addr = vga_putd(addr, va_arg(vargs, int), attr);
            else if (c == 'x')
                addr = vga_putx(addr, (uint32_t)va_arg(vargs, int), attr);
        }
    }

    va_end(vargs);
}

void vga_cls() {
    void *vga_end = (void *)(VGA_ADDR + VGA_ROWS*VGA_COLS*VGA_CHAR_SIZE);
    void *addr;
    for (addr = (void *)VGA_ADDR; addr < vga_end; addr += VGA_CHAR_SIZE)
        vga_putchar(addr, ' ', 0x0F);

}

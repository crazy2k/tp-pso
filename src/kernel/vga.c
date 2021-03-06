#include <i386.h>
#include <tipos.h>
#include <stdarg.h>
#include <vga.h>
#include <utils.h>

#define VGA_TEMPBUFSIZE 40
#define VGA_UI_BASE 16
#define VGA_UI_LOG2BASE 4
#define VGA_UI_REPLENGTH (sizeof(uint32_t)*8/VGA_UI_LOG2BASE)


uint_16 vga_port = 0x3D0;


void vga_init(void) {
//    vga_update_cursor(0,0);
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


void *vga_putd(void *addr, long int num, uint8_t attr) {
    static char buf[VGA_TEMPBUFSIZE];
    itostr(num, buf);

    return vga_puts(addr, buf, attr);
}


void *vga_putx(void *addr, uint32_t n, uint8_t attr) {
    char str[VGA_UI_REPLENGTH + 1];

    itohex(n, str);

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

void vga_clear(void *begin, void *end) {
    void *addr;
    for (addr = begin; addr < end; addr += VGA_CHAR_SIZE)
        vga_putchar(addr, ' ', 0x0F);
}

void vga_cls() {
    void *vga_end = (void *)(VGA_ADDR + VGA_ROWS*VGA_COLS*VGA_CHAR_SIZE);
    vga_clear((void *)VGA_ADDR, vga_end);
}

void vga_clline(void *pos) {
    vga_clear(pos, pos + VGA_ROW_SIZE);
}

void vga_update_cursor(int row, int col)
{
    uint8_t position = (row*80) + col;

    // cursor LOW port to vga INDEX register
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(position&0xFF));
    // cursor HIGH port to vga INDEX register
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((position>>8)&0xFF));
}

#include <i386.h>
#include <tipos.h>
#include "vga.h"

uint_16 vga_port = 0x3D0;

void vga_init(void) {
}

void vga_write(uint_16 row, uint_16 col, const char* msg, uint_8 attr) {
    char c;
    while ((c = *msg++) != '\0')
        vga_writechar(row, col++, c, attr);
}

void vga_writechar(uint16_t row, uint16_t col, const char chr, uint8_t attr) {
    uint8_t *bytepos = (uint8_t *)VGA_POS_ADDR(row, col);
    bytepos[0] = chr;
    bytepos[1] = attr;
}

void vga_printf(uint_16 f, uint_16 c, const char* format, uint_8 attr, ...) {
}

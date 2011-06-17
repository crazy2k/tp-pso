#ifndef __VGA_H__
#define __VGA_H__

#include <tipos.h>

void vga_init(void);

void *vga_putchar(void *addr, const char chr, uint8_t attr);
void vga_write(uint_16 f, uint_16 c, const char* msg, uint_8 attr);
void vga_writechar(uint16_t row, uint16_t col, const char chr, uint8_t attr);
void vga_printf(uint_16 f, uint_16 c, const char* format, uint_8 attr, ...) __attribute__ ((format (printf, 3, 5)));
void vga_cls();
void vga_clline(void *pos);
void update_cursor(int row, int col);

#define VGA_ADDR 0xB8000ul
#define VGA_CHAR_SIZE 2
#define VGA_ROWS 25
#define VGA_COLS 80
#define VGA_SCREEN_SIZE VGA_ROWS*VGA_COLS*VGA_CHAR_SIZE
#define VGA_ROW_SIZE (VGA_COLS*VGA_CHAR_SIZE)

#define VGA_POS_ADDR(row, col) \
    ((void *)(VGA_ADDR + row*VGA_COLS*VGA_CHAR_SIZE + col*VGA_CHAR_SIZE))

#define VGA_LINE_BEGIN(pos, screen) \
    (void *)(((((pos) - (screen))/VGA_ROW_SIZE)*\
        VGA_ROW_SIZE) + (screen))


/* Paleta de 16 colores */
#define VGA_FC_BLACK   0x00
#define VGA_FC_BLUE    0x01
#define VGA_FC_GREEN   0x02
#define VGA_FC_RED     0x03
#define VGA_FC_CYAN    0x04
#define VGA_FC_MAGENTA 0x05
#define VGA_FC_BROWN   0x06
#define VGA_FC_WHITE   0x07

#define VGA_FC_LIGHT   0x08
#define VGA_FC_BLINK   0x80

#define VGA_BC_BLACK   0x00
#define VGA_BC_BLUE    0x10
#define VGA_BC_GREEN   0x20
#define VGA_BC_CYAN    0x30
#define VGA_BC_RED     0x40
#define VGA_BC_MAGENTA 0x50
#define VGA_BC_BROWN   0x60
#define VGA_BC_WHITE   0x70

#endif


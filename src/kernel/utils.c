#include <utils.h>
#include <vga.h>
#include <i386.h>

#define PANIC_PREFIX "PANIC : "

void custom_kpanic_msg(char* custom_msg) {
    cli(); 
    vga_write(0, 0, PANIC_PREFIX,  VGA_BC_RED | VGA_FC_WHITE | VGA_FC_LIGHT); 
    vga_write(0, sizeof(PANIC_PREFIX), custom_msg,  VGA_BC_RED | VGA_FC_WHITE | VGA_FC_LIGHT); 
    hlt(); 
    while(1);
}

uint32_t disable_interrupts() {
    uint32_t eflags;
    __asm__ __volatile__("pushf");
    __asm__ __volatile__("pop %0" : "=a" (eflags) :);
    __asm__ __volatile__("cli");

    return eflags;
}

void restore_eflags(uint32_t eflags) {
    __asm__ __volatile__("push %0" : : "r" (eflags));
    __asm__ __volatile__("popf");
}

char sc2ascii_table[] = {0xFF, 27, '1', '2', '3', '4', '5', '6', '7',
    '8', '9', '0', '-', '=', 8, 9, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
    'o', 'p', '[', ']', 13, 2, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
    ';', 39, '`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 1,
    150, 4, ' ', 12, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 11, 10,
    147, 148, 149, 153, 144, 145, 146, 151, 141, 142, 143, 140, 154, 14, 255,
    255, 138, 139, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 152, 3, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 6, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 155, 7, 15, 5, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 16, 17, 29 ,30, 31, 255, 26, 255, 28, 255, 23, 24, 25, 22,
    0x7f, 255, 255, 255, 255, 255, 255, 255, 20, 21, 18, 255, 255};

char sc2ascii(unsigned char sc) {
    int idx = (int)sc;
    return sc2ascii_table[idx];
}

int read_from_circ_buff(char* dst, circular_buf_t *cbuf, uint32_t size,
    uint32_t buf_size) {

    uint32_t n = (size < cbuf->remaining) ? size : cbuf->remaining;

    char *kb_cbuf = (char *)cbuf->buf;

    int i;
    for(i = 0; i < n; i++) 
        dst[i] = kb_cbuf[(cbuf->offset - cbuf->remaining + i) % buf_size]; 

    cbuf->remaining -= n;

    return i;
}

void put_char_to_circ_buff(circular_buf_t *cbuf, char src, uint32_t buf_size) {
    write_to_circ_buff(cbuf, &src, 1, buf_size);
}


int write_to_circ_buff(circular_buf_t *cbuf, char *src, uint32_t size, uint32_t buf_size) {
    char *kb_cbuf = (char *)cbuf->buf;

    int i;
    for(i = 0; i < size; i++) 
      kb_cbuf[(cbuf->offset + i) % buf_size] = src[i];

    cbuf->remaining += size;

    if (cbuf->remaining >= buf_size)
        cbuf->remaining = buf_size;

    cbuf->offset = (cbuf->offset + size) % buf_size;

    return i;
}

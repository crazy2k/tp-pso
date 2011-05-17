#include <utils.h>
#include <vga.h>
#include <i386.h>

#define PANIC_PREFIX "PANIC : "

void *memcpy(void *dest, const void *src, size_t n) {
    char *bdest = (char *) dest;
    char *bsrc = (char *) src;
    int i;

    for (i = 0; i < n; i++)
        bdest[i] = bsrc[i];

    return dest;
}

void *memset(void *s, int c, size_t n) {
    char *bs = (char *)s;
    int i;

    for (i = 0; i < n; i++)
        bs[i] = (char)c;

    return s;
}

void custom_kpanic_msg(char* custom_msg) {
    cli(); 
    vga_write(0, 0, PANIC_PREFIX,  VGA_BC_RED | VGA_FC_WHITE | VGA_FC_LIGHT); 
    vga_write(0, sizeof(PANIC_PREFIX), custom_msg,  VGA_BC_RED | VGA_FC_WHITE | VGA_FC_LIGHT); 
    hlt(); 
    while(1);
}


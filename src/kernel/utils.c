#include <utils.h>
#include <vga.h>
#include <hdd.h>
#include <i386.h>

#define PANIC_PREFIX "PANIC : "
#define READ_BUF 4096

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


int read_from_blockdev(blockdev *bdev, uint64_t offset, void *buf, uint32_t size) {    
    static char read_buf[READ_BUF];

    uint32_t remainder = offset % bdev->size,
             pos = offset / bdev->size,
             last_remainder = bdev->size - ((size + remainder) % bdev->size),
             sectors_size = size + remainder + last_remainder,
             middle_sectors_size = sectors_size - remainder - last_remainder;

    bdev->read(bdev, pos, read_buf, bdev->size);
    memcpy(buf, read_buf + remainder, bdev->size - remainder);

    if (size - (bdev->size - remainder) >= bdev->size)
        bdev->read(bdev, pos + 1, buf + bdev->size - remainder, middle_sectors_size);
    

    if (remainder + size > bdev->size && last_remainder > 0) {
        bdev->read(bdev, (sectors_size/bdev->size) - 1, read_buf, bdev->size);
        memcpy(buf + (size - (bdev->size - last_remainder)), read_buf, (bdev->size - last_remainder));
    }
    
    return size;
}

#include <swap.h>
#include <errors.h>
#include <debug.h>
#include <utils.h>
#include <i386.h>
#include <fs.h>

// Swap file
static chardev *sf;

void swap_init(void) {
//    debug_printf("* swap: init\n");
//    sf = fs_open("/disk/bleh", FS_OPEN_RDWR);
//    char c;
//    sf->seek(sf, 1024);
//    sf->read(sf, &c, 1);
//    debug_printf("* swap: data read: %x\n", c);
}

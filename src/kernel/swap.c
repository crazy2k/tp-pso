#include <swap.h>
#include <errors.h>
#include <debug.h>
#include <utils.h>
#include <i386.h>
#include <fs.h>
#include <mm.h>

// Swap file
static chardev *sf;
static uint32_t next_id = 0;

void swap_init(void) {
    debug_printf("* swap: init\n");
    sf = fs_open("/disk/psoswap", FS_OPEN_RDWR);
}

uint32_t swap_unload(void *vaddr) {
    uint32_t id = next_id;
    sf->seek(sf, id*PAGE_SIZE);
    sf->write(sf, vaddr, PAGE_SIZE);
    debug_printf("* swap: data unloaded: %x\n");
    next_id++;
    return id;
}

void swap_load(uint32_t id, void *vaddr) {
    sf->seek(sf, id*PAGE_SIZE);
    sf->read(sf, vaddr, PAGE_SIZE);
    debug_printf("* swap: data loaded: %x\n");
}

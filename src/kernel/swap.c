#include <swap.h>
#include <errors.h>
#include <debug.h>
#include <utils.h>
#include <i386.h>
#include <fs.h>
#include <mm.h>

chardev *get_cdev_for_id(uint32_t id);
uint32_t get_pos_for_id(uint32_t id);

// Swap file
static SWAP_FILES_COUNT = 50;
static chardev *sf[50];
static char filename_buf[20];
static char num_buf[20];
static uint32_t next_id = 0;
static char *BASE_FILENAME = "/disk/psoswap";

void swap_init(void) {
    debug_printf("* swap: init\n");
    for (int i = 0; i < SWAP_FILES_COUNT; i++) {
        itostr(i, num_buf);
        strcpy(filename_buf, BASE_FILENAME);
        strcat(filename_buf, num_buf);
        debug_printf("* swap: opening file %s\n",filename_buf);
        sf[i] = fs_open(filename_buf, FS_OPEN_RDWR);
    }
}

uint32_t swap_unload(void *vaddr) {
    uint32_t id = next_id;
    chardev *cdev = get_cdev_for_id(next_id);
    uint32_t pos = get_pos_for_id(next_id);
    cdev->seek(cdev, pos);
    cdev->write(cdev, vaddr, PAGE_SIZE);
    debug_printf("* swap: data unloaded with id %d\n", id);
    next_id++;
    return id;
}

void swap_load(uint32_t id, void *vaddr) {
    chardev *cdev = get_cdev_for_id(id);
    uint32_t pos = get_pos_for_id(id);
    cdev->seek(cdev, pos);
    cdev->read(cdev, vaddr, PAGE_SIZE);
    debug_printf("* swap: data loaded to address %x (id: %d)\n", vaddr, id);
}

chardev *get_cdev_for_id(uint32_t id) {
    debug_printf("* swap: cdev chosen is %d\n", id/3);
    return sf[id/3];
}

uint32_t get_pos_for_id(uint32_t id) {
    debug_printf("* swap: pos chosen is %d\n", (id % 3)*PAGE_SIZE);
    return (id % 3)*PAGE_SIZE;
}
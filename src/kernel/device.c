#include <device.h>
#include <errors.h>
#include <loader.h>
#include <utils.h>
#include <debug.h>

#define READ_BUF 4096

int device_descriptor(chardev *dev) {
    return loader_add_file(dev);
}

void device_init(void) {
}

void* operate_with_bdev(blockdev *bdev, bd_addr_t addr, void *buf, int size, bool write) { 
    static char read_buf[READ_BUF];
    
    uint32_t first_remainder = (bdev->size - addr.offset) % bdev->size,
             first_lineal_sector = addr.sector + (addr.offset ? 1 : 0),
             lineal_sectors_size = align_to_lower(size - first_remainder, bdev->size),
             last_remainder_sector = first_lineal_sector + (lineal_sectors_size / bdev->size);

    debug_printf("** operate_with_bdev: %s: size: %x\n",
                 write == TRUE ? "write" : "read",
                 size);
    
    if (first_remainder > 0) {
        if (write == TRUE) {
            bdev->write(bdev, addr.sector, buf, size);
        } else {
            bdev->read(bdev, addr.sector, read_buf, bdev->size);
            memcpy(buf, read_buf + addr.offset, min(first_remainder, size));
        }
        size -= first_remainder;
        debug_printf("** read_from_bdev(): first_remainder > 0: "
            "first_rem: %x, size: %x\n", first_remainder, size);
    }
    if (size > 0 && lineal_sectors_size > 0) {
        if (write == TRUE) {
            bdev->write(bdev, first_lineal_sector, buf + first_remainder, lineal_sectors_size);
        } else {
            bdev->read(bdev, first_lineal_sector, buf + first_remainder, lineal_sectors_size);
        }
        size -= lineal_sectors_size;
        debug_printf("** read_from_bdev(): size > 0 && lineal ... > 0: "
            "size: %x\n", size);
    }
    if (size > 0) {
        if (write == TRUE) {
            bdev->write(bdev, last_remainder_sector, buf + first_remainder + lineal_sectors_size, bdev->size);
        } else {
            bdev->read(bdev, last_remainder_sector, read_buf, bdev->size);
            memcpy(buf + first_remainder + lineal_sectors_size, read_buf, size);
        }

        debug_printf("** read_from_bdev(): size > 0: "
            "size: %x\n", size);
    }

    debug_printf("** read_from_bdev(): buffer: ");
    uint32_t *buf32 = buf;
    int i;
    for (i = 0; i < 128/4; i++)
        debug_printf("%x ", *(buf32 + i));
    debug_printf("\n");

    return buf;
}
#include <device.h>
#include <errors.h>
#include <loader.h>
#include <utils.h>

#define READ_BUF 4096

int device_descriptor(chardev *dev) {
    return loader_add_file(dev);
}

void device_init(void) {
}

void* read_from_bdev(blockdev *bdev, bd_addr_t addr, void *buf, int size) { 
    static char read_buf[READ_BUF];
    
    uint32_t first_remainder = (bdev->size - addr.offset) % bdev->size,
             first_lineal_sector = addr.sector + (addr.offset ? 1 : 0),
             lineal_sectors_size = align_to_lower(size - first_remainder, bdev->size),
             last_remainder_sector = first_lineal_sector + (lineal_sectors_size / bdev->size);
    
    if (first_remainder > 0) {
        bdev->read(bdev, addr.sector, read_buf, bdev->size);
        memcpy(buf, read_buf, min(size, first_remainder));
        size -= first_remainder;
    }
    if (size > 0 && lineal_sectors_size > 0) {
        bdev->read(bdev, first_lineal_sector, buf + first_remainder, lineal_sectors_size);
        size -= lineal_sectors_size;
    }
    if (size > 0) {
        bdev->read(bdev, last_remainder_sector, read_buf, bdev->size);
        memcpy(buf + first_remainder + lineal_sectors_size, read_buf, size);
    }

    return buf;
}

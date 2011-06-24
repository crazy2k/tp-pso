#include <device.h>
#include <errors.h>
#include <loader.h>

#define READ_BUF 4096

int device_descriptor(chardev *dev) {
    return loader_add_file(dev);
}

void device_init(void) {
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

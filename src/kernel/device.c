#include <device.h>
#include <errors.h>
#include <loader.h>


int device_descriptor(chardev *dev) {
    return loader_add_file(dev);
}

void device_init(void) {
}
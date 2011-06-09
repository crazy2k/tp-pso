#ifndef __SERIAL_H__
#define __SERIAL_H__

#include <tipos.h>
#include <device.h>

#ifdef __KERNEL__

typedef struct serial_chardev serial_chardev;
struct serial_chardev {
    uint32_t clase;
    uint32_t refcount;
    chardev_flush_t* flush;
    chardev_read_t* read;
    chardev_write_t* write;
    chardev_seek_t* seek;

    uint16_t port;

    void *buf;
    uint32_t buf_offset;
    uint32_t buf_remaining;
    uint32_t buf_status;

    serial_chardev *next;
    serial_chardev *prev;
}__attribute__((__packed__));


void serial_init();

chardev* serial_open(int nro);

void serial_recv();

#endif

#endif

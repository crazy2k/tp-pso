#ifndef __SERIAL_H__
#define __SERIAL_H__

#include <tipos.h>
#include <device.h>
#include <utils.h>

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

    circular_buf_t buf;
    int waiting_process;
}__attribute__((__packed__));


void serial_init();

chardev* serial_open(int nro);

void serial_recv();

#endif

#endif

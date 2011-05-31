#ifndef __CON_H__
#define __CON_H__

#include <tipos.h>
#include <device.h>

typedef struct con_chardev con_chardev;
struct con_chardev {
    uint32_t clase;
    uint32_t refcount;
    chardev_flush_t* flush;
    chardev_read_t* read;
    chardev_write_t* write;
    chardev_seek_t* seek;

    void *screen;
    void *screen_limit;
    void *current_pos;
    uint8_t current_attr;

    con_chardev *next;
    con_chardev *prev;
}__attribute__((__packed__));

void con_init();

chardev* con_open(void);

void con_focus(con_chardev *con);
con_chardev *con_get_current_console();

#endif

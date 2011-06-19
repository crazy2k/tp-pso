#ifndef __HDD_H__
#define __HDD_H__


#ifdef __KERNEL__
#include <tipos.h>
#include <device.h>
#include <utils.h>

enum type {
    PRIMARY_MASTER,
    PRIMARY_SLAVE,
    SECONDARY_MASTER,
    SECONDARY_SLAVE
};

typedef struct hdd_blockdev hdd_blockdev;
struct hdd_blockdev {
	uint32_t clase;
	uint32_t refcount;
	blockdev_flush_t *flush;
	blockdev_read_t *read;
	blockdev_write_t *write;
	uint_32 size;

    enum type type;

    circular_buf_t buf;
    int waiting_process;
}__attribute__((__packed__));


blockdev* hdd_open(int nro);
sint_32 hdd_block_read(blockdev *this, uint32_t pos, void *buf,
    uint32_t size);

void hdd_init(void);

void hdd_recv_primary();

#endif

#endif


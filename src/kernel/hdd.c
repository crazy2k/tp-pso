#include <hdd.h>
#include <utils.h>
#include <i386.h>
#include <loader.h>
#include <mm.h>
#include <vga.h>
#include <debug.h>


#define MAX_HDD_BLOCKDEVS 4

#define BUF_SIZE 4096

#define DEFAULT_SECTOR_SIZE 512
/*
#define PRIMARY_MASTER_ID   0
#define PRIMARY_SLAVE_ID    1
#define SECONDARY_MASTER_ID 2
#define SECONDARY_SLAVE_ID  3
*/

// Direcciones base para los puertos de control de los buses ATA
#define PRIMARY_BASE    0x01F0
#define SECONDARY_BASE  0x0170

// Offsets
#define PORT_DATA           0
#define PORT_FEATURES       1
#define PORT_SECTOR_COUNT   2
#define PORT_SECTOR_NUMBER  3
#define PORT_CYLINDER_LO    4
#define PORT_CYLINDER_HI    5
#define PORT_DRIVE          6
#define PORT_COMMAND        7

#define COMMAND_READ_SECTORS    0x20
#define COMMAND_WRITE_SECTORS   0x30

// Puertos de los Control Registers/Alternate Status (segun se lea o se
// escriba)
#define PRIMARY_DEV_CTRL    0x03F6
#define PRIMARY_DEV_ALTST   PRIMARY_DEV_CTRL
#define SECONDARY_DEV_CTRL  0x0376
#define SECONDARY_DEV_ALTST SECONDARY_DEV_CTRL


#define LBA_HIGHEST_4BITS(lba) ((uint8_t)(((lba) >> 24) & 0x0F))


static void initialize_hdd_blockdev(hdd_blockdev *hbdev, uint32_t id, uint32_t sector_size);
static void hdd_recv(hdd_blockdev *hbdev);
static void hdd_send(hdd_blockdev *hbdev);
static sint_32 hdd_block_read_sectors(hdd_blockdev *this, uint32_t pos, void *buf, uint32_t size);
static sint_32 hdd_block_write_sectors(hdd_blockdev *hbdev, uint32_t pos, const void *buf, uint32_t size);

static hdd_blockdev hdd_blockdevs[MAX_HDD_BLOCKDEVS];


void hdd_init(void) {
    memset(hdd_blockdevs, 0, sizeof(hdd_blockdevs));

    initialize_hdd_blockdev(&hdd_blockdevs[PRIMARY_MASTER],
        PRIMARY_MASTER, DEFAULT_SECTOR_SIZE);
}

sint_32 hdd_block_write(blockdev* this, uint_32 pos, const void* buf, uint_32 size) {
    if (this->clase != DEVICE_HDD_BLOCKDEV)
        return -1;

    hdd_blockdev *hbdev = (hdd_blockdev *)this;

    sint_32 written_chars = 0;

    debug_printf("** hdd_block_write: start\n");

    sem_wait(&hbdev->sem);
        hbdev->current_command = COMMAND_WRITE_SECTORS;
        while (written_chars < size)
            written_chars += hdd_block_write_sectors(hbdev,
                pos + (written_chars/hbdev->size),
                buf + written_chars, size - written_chars);
    sem_signaln(&hbdev->sem);

    debug_printf("** hdd_block_write: read: %x\n", written_chars);

    return size;
}


#define GET_BASE(hbdev) \
    ({ \
        uint16_t _base = NULL; \
        switch ((hbdev)->type) { \
            case PRIMARY_MASTER: \
            case PRIMARY_SLAVE: \
                _base = PRIMARY_BASE; \
                break; \
            case SECONDARY_MASTER: \
            case SECONDARY_SLAVE: \
                _base = SECONDARY_BASE; \
                break; \
        } \
        _base; \
    })

#define IS_MASTER(hbdev) \
    ({ \
        bool _is_master = FALSE; \
        switch (hbdev->type) { \
            case PRIMARY_MASTER: \
            case SECONDARY_MASTER: \
                _is_master = TRUE; \
                break; \
            case PRIMARY_SLAVE: \
            case SECONDARY_SLAVE: \
                _is_master = FALSE; \
                break; \
        } \
        _is_master; \
    })


/* Lee los sectores necesarios. Parte del sector indicado por el LBA de 28
 * bits en ``pos`` y lee ``size`` bytes (se asume que ``size`` es un multiplo
 * del tamanio del sector del dispositivo).
 */
sint_32 hdd_block_read(blockdev *this, uint32_t pos, void *buf,
    uint32_t size) {

    if (this->clase != DEVICE_HDD_BLOCKDEV)
        return -1;

    hdd_blockdev *hbdev = (hdd_blockdev *)this;

    sint_32 read_chars = 0;

    debug_printf("** hdd_block_read: start\n");

    sem_wait(&hbdev->sem);
        hbdev->current_command = COMMAND_READ_SECTORS;
        while (read_chars < size)
            read_chars += hdd_block_read_sectors(hbdev,
                pos + (read_chars/hbdev->size),
                buf + read_chars, size - read_chars);
    sem_signaln(&hbdev->sem);

    debug_printf("** hdd_block_read: read: %x\n", read_chars);

    return size;
}

/* Lee la cantidad de bytes especificados en size; si estos no caben en el 
 * buffer solo lee hasta llenar el buffer del dispositivo.
 */
sint_32 hdd_block_read_sectors(hdd_blockdev *hbdev, uint32_t pos, void *buf,
    uint32_t size) {

    size = min(BUF_SIZE, size);

    uint32_t lba = pos & __LOW28_BITS__;
    uint16_t base = GET_BASE(hbdev);

    uint32_t drive_data = (IS_MASTER(hbdev)) ? 0xE0 : 0xF0;
    outb(base + PORT_DRIVE, LBA_HIGHEST_4BITS(lba) | drive_data);

    outb(base + PORT_FEATURES, NULL);

    outb(base + PORT_SECTOR_COUNT, size/hbdev->size);

    outb(base + PORT_SECTOR_NUMBER, (uint8_t)lba);
    outb(base + PORT_CYLINDER_LO, (uint8_t)(lba >> 8));
    outb(base + PORT_CYLINDER_HI, (uint8_t)(lba >> 16));

    outb(base + PORT_COMMAND, COMMAND_READ_SECTORS);

    debug_printf("hdd_block_read_sectors: request\n");

    debug_printf("** hdd_block_read_sectors: t = %x\n",
        (uint32_t)hbdev - (uint32_t)hdd_blockdevs);

    while (hbdev->buf.remaining == 0) {
        debug_printf("hdd_block_read_sectors: loop\n");
        loader_enqueue(&hbdev->waiting_process);
    }

    debug_printf("hdd_block_read_sectors: awake\n");

    return read_from_circ_buff((char *)buf, &hbdev->buf, size, BUF_SIZE);
}

sint_32 hdd_block_write_sectors(hdd_blockdev *hbdev, uint32_t pos, const void *buf,
    uint32_t size) {

    size = min(BUF_SIZE, size);

    uint32_t lba = pos & __LOW28_BITS__;
    uint16_t base = GET_BASE(hbdev);

    uint32_t drive_data = (IS_MASTER(hbdev)) ? 0xE0 : 0xF0;
    outb(base + PORT_DRIVE, LBA_HIGHEST_4BITS(lba) | drive_data);

    outb(base + PORT_FEATURES, NULL);

    outb(base + PORT_SECTOR_COUNT, size/hbdev->size);

    outb(base + PORT_SECTOR_NUMBER, (uint8_t)lba);
    outb(base + PORT_CYLINDER_LO, (uint8_t)(lba >> 8));
    outb(base + PORT_CYLINDER_HI, (uint8_t)(lba >> 16));

    outb(base + PORT_COMMAND, COMMAND_WRITE_SECTORS);

    debug_printf("hdd_block_write_sectors: request\n");

    debug_printf("** hdd_block_write_sectors: t = %x\n",
        (uint32_t)hbdev - (uint32_t)hdd_blockdevs);

    write_to_circ_buff(&hbdev->buf, (char *)buf, size, BUF_SIZE);

    while (hbdev->buf.remaining != 0) {
        debug_printf("hdd_block_write_sectors: loop\n");
        loader_enqueue(&hbdev->waiting_process);
    }

    debug_printf("hdd_block_write_sectors: awake\n");

    return size;
	
}

blockdev *hdd_open(int no) {
    if (no < 0 || no > 3)
        return NULL;

    return (blockdev *)(&hdd_blockdevs[no]);
}

static void initialize_hdd_blockdev(hdd_blockdev *hbdev, uint32_t type, uint32_t sector_size) {
    hbdev->clase = DEVICE_HDD_BLOCKDEV;
    hbdev->refcount = 0;
    hbdev->flush = NULL;
    hbdev->read = hdd_block_read;
    hbdev->write = hdd_block_write;
    hbdev->size = sector_size;
    hbdev->type = type;
    hbdev->waiting_process = -1;
    hbdev->buf = ((circular_buf_t) {
        .buf = mm_mem_kalloc(),
        .offset = 0,
        .remaining = 0
    });
    hbdev->sem = SEM_NEW(1);
    hbdev->current_command = NULL;
}


void hdd_primary_isr() {
    uint8_t drive = inb(PRIMARY_BASE + PORT_DRIVE);

    enum type t;
    if (drive & 1)
        t = PRIMARY_SLAVE;
    else
        t = PRIMARY_MASTER;

    hdd_blockdev *hbdev = &hdd_blockdevs[t];

    debug_printf("** hdd_primary_isr: t = %x\n", t);
    uint16_t base = GET_BASE(hbdev);

    uint8_t cmd = hbdev->current_command;
    debug_printf("** hdd_primary_isr: cmd = %x\n", cmd);
    if (cmd == COMMAND_READ_SECTORS) {
        hdd_recv(hbdev);
    } else if (cmd == COMMAND_WRITE_SECTORS) {
        hdd_send(hbdev);
    }
}

static void hdd_recv(hdd_blockdev *hbdev) {
    uint16_t base = GET_BASE(hbdev);

    int i;
    for (i = 0; i < (hbdev->size/sizeof(uint16_t)); i++) {
        uint16_t w = inw(base + PORT_DATA);
        put_char_to_circ_buff(&hbdev->buf, (uint8_t)w, BUF_SIZE);
        put_char_to_circ_buff(&hbdev->buf, (uint8_t)(w >> 8), BUF_SIZE);
    }

    debug_printf("hdd_recv: unqueue\n");

    loader_unqueue(&hbdev->waiting_process);
}

static void hdd_send(hdd_blockdev *hbdev) {
    uint16_t base = GET_BASE(hbdev);

    uint16_t *buf = mm_mem_kalloc();

    // Escribimos un sector al disco
    int i;
    for (i = 0; i < (hbdev->size/sizeof(uint16_t)); i++) {
        read_from_circ_buff((char *)buf, &hbdev->buf, sizeof(uint16_t), sizeof(uint16_t));
        outw(base + PORT_DATA, *buf);
    }

    mm_mem_free(buf);

    debug_printf("hdd_send: unqueue\n");

    loader_unqueue(&hbdev->waiting_process);
}

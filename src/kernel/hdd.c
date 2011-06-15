#include <hdd.h>
#include <utils.h>
#include <i386.h>
#include <loader.h>
#include <mm.h>


#define MAX_HDD_BLOCKDEVS 4

#define BUF_SIZE 4096

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

// Puertos de los Control Registers/Alternate Status (segun se lea o se
// escriba)
#define PRIMARY_DEV_CTRL    0x03F6
#define PRIMARY_DEV_ALTST   PRIMARY_DEV_CTRL
#define SECONDARY_DEV_CTRL  0x0376
#define SECONDARY_DEV_ALTST SECONDARY_DEV_CTRL


static void initialize_hdd_blockdev(hdd_blockdev *hbdev, uint32_t id);
static void hdd_recv(hdd_blockdev *hbdev);

static hdd_blockdev hdd_blockdevs[MAX_HDD_BLOCKDEVS];


void hdd_init(void) {
    memset(hdd_blockdevs, 0, sizeof(hdd_blockdevs));

    initialize_hdd_blockdev(&hdd_blockdevs[PRIMARY_MASTER],
        PRIMARY_MASTER);
}

sint_32 hdd_block_write(blockdev* this, uint_32 pos, const void* buf, uint_32 size) {
    return -1; //Opcional
}

static uint16_t get_base(hdd_blockdev *hbdev) {
    switch (hbdev->type) {
        case PRIMARY_MASTER:
        case PRIMARY_SLAVE:
            return PRIMARY_BASE;
        case SECONDARY_MASTER:
        case SECONDARY_SLAVE:
            return SECONDARY_BASE;
        default:
            return NULL;
    }
}

static bool is_master(hdd_blockdev *hbdev) {
    switch (hbdev->type) {
        case PRIMARY_MASTER:
        case SECONDARY_MASTER:
            return TRUE;
        case PRIMARY_SLAVE:
        case SECONDARY_SLAVE:
            return FALSE;
        default:
            return FALSE;
    }
}

#define LBA_HIGHEST_4BITS(lba) ((uint8_t)(((lba) >> 24) & 0x0F))
// ``pos`` es un LBA de 28 bits
sint_32 hdd_block_read(blockdev *this, uint32_t pos, void *buf,
    uint32_t size) {
    if (this->clase != DEVICE_HDD_BLOCKDEV)
        return -1;

    hdd_blockdev *hbdev = (hdd_blockdev *)this;

    uint32_t lba = pos & __LOW28_BITS__;
    uint16_t base = get_base(hbdev);

    uint32_t drive_data = (is_master(hbdev)) ? 0xE0 : 0xF0;
    outb(base + PORT_DRIVE, LBA_HIGHEST_4BITS(lba) | drive_data);

    outb(base + PORT_FEATURES, NULL);

    // XXX: De momento dejamos 2 sectores para probar
    outb(base + PORT_SECTOR_COUNT, 2);

    outb(base + PORT_SECTOR_NUMBER, (uint8_t)lba);
    outb(base + PORT_CYLINDER_LO, (uint8_t)(lba >> 8));
    outb(base + PORT_CYLINDER_HI, (uint8_t)(lba >> 16));

    outb(base + PORT_COMMAND, COMMAND_READ_SECTORS);

    while (hbdev->buf.remaining == 0)
        loader_enqueue(&hbdev->waiting_process);

    return read_from_circ_buff((char *)buf, &hbdev->buf, size, BUF_SIZE);
}

blockdev *hdd_open(int nro) {
    return 0;
}

static void initialize_hdd_blockdev(hdd_blockdev *hbdev, uint32_t type) {
    hbdev->clase = DEVICE_HDD_BLOCKDEV;
    hbdev->refcount = 0;
    hbdev->flush = NULL;
    hbdev->read = hdd_block_read;
    hbdev->write = hdd_block_write;
    hbdev->size = 512;
    hbdev->type = type;
    hbdev->buf = ((circular_buf_t) {
        .buf = mm_mem_kalloc(),
        .offset = 0,
        .remaining = 0
    });
}

void hdd_recv_primary() {
    uint8_t drive = inb(PRIMARY_BASE + PORT_DRIVE);

    enum type t;
    if (drive & 1)
        t = PRIMARY_SLAVE;
    else
        t = PRIMARY_MASTER;

    hdd_recv(&hdd_blockdevs[t]);
}

static void hdd_recv(hdd_blockdev *hbdev) {
    uint16_t base = GET_BASE(hbdev);

    uint8_t status = inb(base + PORT_COMMAND);

    int i;
    for (i = 0; i < (hbdev->size/sizeof(uint16_t)); i++) {
        uint16_t w = inw(base + PORT_DATA);
        put_char_to_circ_buff(&hbdev->buf, (uint8_t)w, BUF_SIZE);
        put_char_to_circ_buff(&hbdev->buf, (uint8_t)(w >> 8), BUF_SIZE);
    }
}

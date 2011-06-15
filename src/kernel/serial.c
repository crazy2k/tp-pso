#include <serial.h>
#include <tipos.h>
#include <i386.h>
#include <isr.h>
#include <pic.h>
#include <debug.h>
#include <utils.h>
#include <mm.h>
#include <loader.h>

#define BUF_SIZE 1024

#define MAX_SERIAL_CHARDEVS 4

#define COM1_PORT 0x03F8
#define COM2_PORT 0x02F8
#define COM3_PORT 0x03E8
#define COM4_PORT 0x02E8

/* Serial Controler sub-SP_PORTs */
#define PORT_DATA  0 /* R/W - DATA flow */
#define PORT_IER   1 /* R/W - Interrupt Enable Register */
#define PORT_IIR   2 /* R   - Interrupt Id Register */
#define PORT_FCTRL 2 /*   W - FIFO Control */
#define PORT_LCTRL 3 /* R/W - Line Control */
#define PORT_MCTRL 4 /* R/W - MODEM Control */
#define PORT_LSTAT 5 /* R/W - Line Status */
#define PORT_MSTAT 6 /* R/W - MODEM Status */
#define PORT_SCRAT 7 /* R/W - Scratch ¿eh? */
#define PORT_DL_LSB 0 /* Divisor latch - LSB (need DLAB=1) */
#define PORT_DL_MSB 1 /* Divisor latch - MSB (need DLAB=1)  */

/*** REMEMBER: Don't use drugs while designing a chip:
 * 
 * 8.10 SCRATCHPAD REGISTER
 * This 8-bit Read Write Register does not control the UART
 * in anyway It is intended as a scratchpad register to be used
 * by the programmer to hold data temporarily
 */

/* Line Control Bits (type selection, and DLAB) */
#define LC_BIT5 0x00
#define LC_BIT6 0x01
#define LC_BIT7 0x02
#define LC_BIT8 0x03

#define LC_PARITY   0x08
#define LC_PARITY_E 0x10
#define LC_PARITY_O 0x00
#define LC_PARITY_ST 0x20
#define LC_BREAK    0x40
#define LC_DLAB     0x80

/* Line Status Bits */
#define LS_DR   0x01  /* Data Ready */
#define LS_OE   0x02  /* Overrun Error indicator (reading) */
#define LS_PE   0x04  /* Parity Error */
#define LS_FE   0x08  /* Framing Error (invalid stop bit) */
#define LS_BI   0x10  /* Break Interrupt */
#define LS_THRE 0x20  /* Transmitter Holding Register Empty */
#define LS_TEMT 0x40  /* Transmitter Empty */

/* FIFO Control Register Bits */
#define FC_FIFO       0x01 /* Enable FIFO (in y out) */
#define FC_CL_RCVFIFO 0x02 /* Clear RCVR FIFO */
#define FC_CL_XMTFIFO 0x04 /* Clear XMIT FIFO */
#define FC_MODE1      0x08 /* No tengo ni la más puta idea de qué hace este bit */
#define FC_TRIGGER_01 0x00 /* Trigger at 1-byte */
#define FC_TRIGGER_04 0x40 /* Trigger at 4-bytes */
#define FC_TRIGGER_08 0x80 /* Trigger at 8-bytes */
#define FC_TRIGGER_14 0xC0 /* Trigger at 14-bytes */

/* Iterrupt Id Bits */
#define II_nINTPEND  0x01 /* No interrupt pending */
#define II_ID_MASK   0x0E /* Mascara de IDs */
#define II_ID_RLS    0x06 /* Int ID: Receiver Line Status Interrupt */
#define II_ID_RDA    0x04 /* Int ID: Receiver Data Available */
#define II_ID_CTI    0x0C /* Int ID: Character Timeout Indicator */
#define II_ID_THRE   0x02 /* Int ID: Transmitter Holding Register Empty */
#define II_ID_MODEM  0x00 /* Int ID: MODEM Status */
#define II_INT_TOUT  0x08

/* Interrupt Enable Bits */
#define IE_RDA   0x01 /* Int Enable: Receiver Data Available */
#define IE_THRE  0x02 /* Int Enable: Transmitter Holding Register Empty */
#define IE_RLS   0x04 /* Int Enable: Receiver Line Status */
#define IE_MODEM 0x08 /* Int Enable: MODEM Status */

#define BUF_SIZE 1024

static serial_chardev serial_chardevs[MAX_SERIAL_CHARDEVS];

static void initialize_serial_chardev(serial_chardev *scdev, uint16_t port);
static void initialize_serial_port(uint16_t port);
static void recv_byte(serial_chardev *scdev);


void serial_init() {
    memset(serial_chardevs, 0, sizeof(serial_chardevs));

    initialize_serial_chardev(&serial_chardevs[0], COM1_PORT);
}

sint_32 serial_read(chardev* this, void* buf, uint_32 size) {
    if (this->clase != DEVICE_SERIAL_CHARDEV)
        return -1;

    serial_chardev *scdev = (serial_chardev *)this;

    while (scdev->buf.remaining == 0)
        loader_enqueue(&scdev->waiting_process);

    return read_from_circ_buff((char *)buf, &scdev->buf, size,
        BUF_SIZE);
}

#define TRANSMITTER_EMPTY(port) (inb((port) + PORT_LSTAT) & 0x20)
sint_32 serial_write(chardev *this, const void *buf, uint_32 size) {
    if (this->clase != DEVICE_SERIAL_CHARDEV)
        return -1;

    serial_chardev *scdev = (serial_chardev *)this;
    uint8_t *cbuf = (uint8_t *)buf;

    int i;
    for (i = 0; i < size; i++) {
        while (!TRANSMITTER_EMPTY(scdev->port));

        outb(scdev->port, cbuf[i]);
    }

	return size;
}

uint_32 serial_flush(chardev* this) {
    if (this->clase != DEVICE_SERIAL_CHARDEV)
        return -1;

    serial_chardev *scdev = (serial_chardev *)this;

    mm_mem_free(scdev->buf.buf);

    return 0;
}

chardev *serial_open(int no) { /* 0 para COM1, 1 para COM2, ... */
    if (no < 0 || no > 3)
        return NULL;

    return (chardev *)(&serial_chardevs[no]);
}

static void initialize_serial_chardev(serial_chardev *scdev, uint16_t port) {
    /*
     * Inicializamos el serial_chardev
     */

    scdev->clase = DEVICE_SERIAL_CHARDEV;
    scdev->refcount = 0;
    scdev->flush = serial_flush;
    scdev->read = serial_read;
    scdev->write = serial_write;

    scdev->port = port;

    scdev->buf = ((circular_buf_t) {
        .buf = mm_mem_kalloc(),
        .offset = 0,
        .remaining = 0
    });

    // Inicializamos el puerto serie
    initialize_serial_port(port);
}

static void initialize_serial_port(uint16_t port) {
    // Desactivamos interrupciones
    outb(port + PORT_IER, 0x00);

    // Encendemos DLAB (Divisor Latch Access Bit)
    outb(port + PORT_LCTRL, LC_DLAB);
    // Escribimos el divisor
    outb(port + PORT_DL_LSB, 0x0C);
    outb(port + PORT_DL_MSB, 0x00);

    // Elegimos: caracteres de 8 bits, 1 stop it, sin parity, sin break.
    outb(port + PORT_LCTRL, LC_BIT8);

    // No usamos los FIFOs
    outb(port + PORT_FCTRL, 0);

    // Marcamos Data Terminal Ready, Request to Send, OUT2
    outb(port + PORT_MCTRL, 0x0B);

    outb(port + PORT_IER, IE_RDA);
}

#define SERIAL_RECVD(port) (inb((port) + PORT_LSTAT) & 1)
void serial_recv() {
    if (SERIAL_RECVD(COM1_PORT)) {
        recv_byte(&serial_chardevs[0]);
    } else if (SERIAL_RECVD(COM2_PORT)) {
        recv_byte(&serial_chardevs[1]);
    } else if (SERIAL_RECVD(COM3_PORT)) {
        recv_byte(&serial_chardevs[2]);
    } else if (SERIAL_RECVD(COM4_PORT)) {
        recv_byte(&serial_chardevs[3]);
    }
}

static void recv_byte(serial_chardev *scdev) {
    if (scdev->clase != DEVICE_SERIAL_CHARDEV) {
        breakpoint();
        return;
    }

    uint8_t b = inb(scdev->port);
    put_char_to_circ_buff(&scdev->buf, b, BUF_SIZE);

    loader_unqueue(&scdev->waiting_process);
}

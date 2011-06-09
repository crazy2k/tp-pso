#include <con.h>
#include <device.h>
#include <vga.h>
#include <utils.h>
#include <mm.h>
#include <debug.h>
#include <loader.h>

#define MAX_CON_CHARDEVS 16
#define KB_BUF_SIZE 1024


// XXX: Deberian ser static?
sint_32 con_read(chardev *this, void *buf, uint_32 size);
sint_32 con_write(chardev *this, const void *buf, uint_32 size);
uint_32 con_flush(chardev *this);


static void scroll_down(con_chardev *ccdev);

static con_chardev con_chardevs[MAX_CON_CHARDEVS];
static con_chardev *free_con_chardevs = NULL;
static con_chardev *open_con_chardevs = NULL;

static con_chardev *current_console = NULL;


void con_init() {
    CREATE_FREE_OBJS_LIST(free_con_chardevs, con_chardevs, MAX_CON_CHARDEVS);
}

chardev* con_open(void) {
    con_chardev *ccdev = POP(&free_con_chardevs);

    ccdev->clase = DEVICE_CON_CHARDEV;
    ccdev->refcount = 0;
    ccdev->flush = con_flush;
    ccdev->read = con_read;
    ccdev->write = con_write;

    ccdev->screen_buf = mm_mem_kalloc();
    ccdev->screen_buf_offset = 0;
    ccdev->focused = FALSE;

    ccdev->kb_buf.buf = mm_mem_kalloc();
    ccdev->kb_buf.offset = 0;
    ccdev->kb_buf.remaining = 0;

    ccdev->waiting_process = -1 ;

    ccdev->current_attr = 0x0F;

    APPEND(&open_con_chardevs, ccdev);

    if (!con_get_current_console())
        con_focus(ccdev);

    return (chardev *)ccdev;
}

sint_32 con_read(chardev *this, void *buf, uint_32 size) {
     if (this->clase != DEVICE_CON_CHARDEV)
        return -1;

    con_chardev *ccdev = (con_chardev *)this;

    while (ccdev->kb_buf.remaining == 0)
        loader_enqueue(&ccdev->waiting_process);

    return copy_from_circ_buff((char *)buf, &ccdev->kb_buf, size, KB_BUF_SIZE);
}

sint_32 con_write(chardev *this, const void *buf, uint_32 size) {
    if (this->clase != DEVICE_CON_CHARDEV)
        return -1;

    con_chardev *ccdev = (con_chardev *)this;
    char *cbuf = (char *)buf;

    void *out_base = (ccdev->focused) ? (void *)VGA_ADDR : ccdev->screen_buf;

    int i;
    for (i = 0; i < size; i++) {
        if (ccdev->screen_buf_offset >= VGA_SCREEN_SIZE)
            scroll_down(ccdev);

        vga_putchar(out_base + ccdev->screen_buf_offset, cbuf[i],
            ccdev->current_attr);
        ccdev->screen_buf_offset += VGA_CHAR_SIZE;
    }
    return i;
}

uint_32 con_flush(chardev *this) {
    if (this->clase != DEVICE_CON_CHARDEV)
        return -1;

    con_chardev *ccdev = (con_chardev *)this;

    mm_mem_free(ccdev->screen_buf);
    mm_mem_free(ccdev->kb_buf.buf);

    UNLINK_NODE(&open_con_chardevs, ccdev);
    APPEND(&free_con_chardevs, ccdev);

    return 0;
}

void con_focus(con_chardev *con) {
    if (current_console == con)
        return;

    if (current_console != NULL) {
        memcpy(current_console->screen_buf, (void *)VGA_ADDR,
            VGA_SCREEN_SIZE);
        current_console->focused = FALSE;
    }

    memcpy((void *)VGA_ADDR, con->screen_buf, VGA_SCREEN_SIZE);
    current_console = con;
    con->focused = TRUE;
}

void con_put_to_kb_buf(con_chardev * ccdev, uint8_t b) {
    ccdev->kb_buf.remaining = (ccdev->kb_buf.remaining == KB_BUF_SIZE) ?
        KB_BUF_SIZE : ccdev->kb_buf.remaining + 1;

    ((uint8_t *)ccdev->kb_buf.buf)[ccdev->kb_buf.offset] = b;

    ccdev->kb_buf.offset = (ccdev->kb_buf.offset + 1) % KB_BUF_SIZE;

    loader_unqueue(&ccdev->waiting_process);
}

con_chardev *con_get_current_console() {
    return current_console;
}

static void scroll_down(con_chardev *ccdev) {
    void *screen = (ccdev->focused) ? (void *)VGA_ADDR : ccdev->screen_buf;
    void *screen_limit = screen + VGA_SCREEN_SIZE;
    memcpy(screen, screen + VGA_ROW_SIZE,
        (screen_limit - screen) - VGA_ROW_SIZE);

    ccdev->screen_buf_offset -= VGA_ROW_SIZE;
    vga_clline(VGA_LINE_BEGIN(screen + ccdev->screen_buf_offset, screen));
}


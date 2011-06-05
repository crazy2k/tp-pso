#include <con.h>
#include <device.h>
#include <vga.h>
#include <utils.h>
#include <mm.h>
#include <debug.h>

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

    ccdev->kb_buf = mm_mem_kalloc();
    ccdev->kb_buf_offset = 0;
    ccdev->kb_buf_remaining = 0;

    ccdev->current_attr = 0x0F;

    APPEND(&open_con_chardevs, ccdev);

    return (chardev *)ccdev;
}

sint_32 con_read(chardev *this, void *buf, uint_32 size) {
     if (this->clase != DEVICE_CON_CHARDEV)
        return -1;

    con_chardev *ccdev = (con_chardev *)this;

    uint32_t rem = ccdev->kb_buf_remaining;
    uint32_t offset = ccdev->kb_buf_offset;

    uint32_t n = (size < rem) ? size : rem;

    char *cbuf = (char *)buf;
    char *kb_cbuf = (char *)ccdev->kb_buf;

    int i;
    for(i = 0; i < n; i++) {
        cbuf[i] = kb_cbuf[(offset - rem + i) % KB_BUF_SIZE];
    }
    //vga_printf(0,0,"n = %x", 0x0F, n);

    return n;
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
    mm_mem_free(ccdev->kb_buf);

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

static void scroll_down(con_chardev *ccdev) {
    void *screen = (ccdev->focused) ? (void *)VGA_ADDR : ccdev->screen_buf;
    void *screen_limit = screen + VGA_SCREEN_SIZE;
    memcpy(screen, screen + VGA_ROW_SIZE,
        (screen_limit - screen) - VGA_ROW_SIZE);

    ccdev->screen_buf_offset -= VGA_ROW_SIZE;
    vga_clline(VGA_LINE_BEGIN(screen + ccdev->screen_buf_offset, screen));
}

con_chardev *con_get_current_console() {
    return current_console;
}

void con_write_to_kb_buf(uint8_t b) {
    con_chardev *ccdev = con_get_current_console();

    ccdev->kb_buf_offset = (ccdev->kb_buf_offset + 1) % KB_BUF_SIZE;
    ccdev->kb_buf_remaining = (ccdev->kb_buf_remaining == KB_BUF_SIZE)?
        KB_BUF_SIZE : ccdev->kb_buf_remaining + 1;

    uint8_t *out = (uint8_t *)(ccdev->kb_buf + ccdev->kb_buf_offset);
    *out = b;
}

#include <con.h>
#include <device.h>
#include <vga.h>
#include <utils.h>
#include <mm.h>

#define MAX_CON_CHARDEVS 16


static void scroll_down(con_chardev *con_cdev);

static con_chardev con_chardevs[MAX_CON_CHARDEVS];
static con_chardev *free_con_chardevs = NULL;
static con_chardev *current_console = NULL;


sint_32 con_read(chardev* this, void* buf, uint_32 size) {
    return 0;
}

sint_32 con_write(chardev* this, const void* buf, uint_32 size) {
    if (this->clase != DEVICE_CON_CHARDEV)
        return -1;

    con_chardev *con_cdev = (con_chardev *)this;
    char *cbuf = (char *)buf;

    int i;
    for (i = 0; i < size; i++) {
        if (con_cdev->current_pos >= con_cdev->screen_limit)
            scroll_down(con_cdev);

        vga_putchar(con_cdev->current_pos, cbuf[i], con_cdev->current_attr);
        con_cdev->current_pos += VGA_CHAR_SIZE;
    }
    return 0;
}

uint_32 con_flush(chardev* this) {
    return 0;
}


chardev* con_open(void) {
    con_chardev *con_cdev = POP(&free_con_chardevs);

    con_cdev->clase = DEVICE_CON_CHARDEV;
    con_cdev->refcount = 0;
    con_cdev->flush = con_flush;
    con_cdev->read = con_read;
    con_cdev->write = con_write;

    con_cdev->screen = mm_mem_kalloc();
    con_cdev->screen_limit = con_cdev->screen +
        VGA_ROWS*VGA_COLS*VGA_CHAR_SIZE;
    con_cdev->current_pos = con_cdev->screen;
    con_cdev->current_attr = 0x0F;
    return NULL;
}

void con_focus(con_chardev *con) {
    current_console = con;
}

void con_init() {
    CREATE_FREE_OBJS_LIST(free_con_chardevs, con_chardevs, MAX_CON_CHARDEVS);
}

static void scroll_down(con_chardev *con_cdev) {
    void *screen = con_cdev->screen;
    void *screen_limit = con_cdev->screen_limit;
    memcpy(screen, screen + VGA_ROW_SIZE,
        (screen_limit - screen) - VGA_ROW_SIZE);

    con_cdev->current_pos -= VGA_ROW_SIZE;
    vga_clline(VGA_LINE_BEGIN(con_cdev->current_pos, screen));
}

con_chardev *con_get_current_console() {
    return current_console;
}

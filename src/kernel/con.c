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


static void update_kb_buf_status(con_chardev *ccdev, uint8_t b);
static void set_status_bit(uint32_t *status, uint32_t mask);
static void unset_status_bit(uint32_t *status, uint32_t mask);
static char sc2kc(uint8_t sc);
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

    ccdev->kb_buf_remaining -= n;

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

    update_kb_buf_status(ccdev, b);

    uint8_t *out = (uint8_t *)(ccdev->kb_buf + ccdev->kb_buf_offset);
    *out = b;
}


#define IS_KEY_RELEASE(b) ((b) & KB_RELEASE_BASE)
#define KB_RELEASE_BASE 0x80
#define KB_KC_SPECIAL_BASE KB_RELEASE_BASE
#define KB_KC_NULL 0x0
#define KB_KC_ERROR (KB_KC_SPECIAL_BASE + 0x0)
#define KB_KC_LSHIFT (KB_KC_SPECIAL_BASE + 0x1)
#define KB_KC_RSHIFT (KB_KC_SPECIAL_BASE + 0x2)
#define KB_KC_NUMLOCK (KB_KC_SPECIAL_BASE + 0x3)
#define KB_KC_SCROLLLOCK (KB_KC_SPECIAL_BASE + 0x4)
#define KB_KC_HOME (KB_KC_SPECIAL_BASE + 0x5)
#define KB_KC_UP (KB_KC_SPECIAL_BASE + 0x6)
#define KB_KC_PGUP (KB_KC_SPECIAL_BASE + 0x7)
#define KB_KC_MINUS (KB_KC_SPECIAL_BASE + 0x8)
#define KB_KC_LEFT (KB_KC_SPECIAL_BASE + 0x9)
#define KB_KC_RIGHT (KB_KC_SPECIAL_BASE + 0xA)
#define KB_KC_END (KB_KC_SPECIAL_BASE + 0xB)
#define KB_KC_DOWN (KB_KC_SPECIAL_BASE + 0xC)
#define KB_KC_PGDN (KB_KC_SPECIAL_BASE + 0xD)
#define KB_KC_INS (KB_KC_SPECIAL_BASE + 0xE)
#define KB_KC_DEL (KB_KC_SPECIAL_BASE + 0xF)
#define KB_KC_SYSRQ (KB_KC_SPECIAL_BASE + 0x10)
#define KB_KC_UNLABELLED (KB_KC_SPECIAL_BASE + 0x11)
#define KB_KC_KEYPAD KB_KC_NULL
#define KB_KC_LALT KB_KC_NULL
#define KB_KC_CAPSLOCK KB_KC_NULL
#define KB_KC_F(i) KB_KC_NULL
#define KB_STATUS_SHIFT 0x0001
static void update_kb_buf_status(con_chardev *ccdev, uint8_t b) {
    void (*update_status)(uint32_t *status, uint32_t mask);

    update_status = set_status_bit;
    if (IS_KEY_RELEASE(b)) {
        b -= KB_RELEASE_BASE;
        update_status = unset_status_bit;
    }

    uint8_t kc = sc2kc(b);
    switch (kc) {
        case KB_KC_LSHIFT:
        case KB_KC_RSHIFT:
            update_status(&ccdev->kb_buf_status, KB_STATUS_SHIFT);
            break;
    }
}

static void set_status_bit(uint32_t *status, uint32_t mask) {
    *status |= mask;
}

static void unset_status_bit(uint32_t *status, uint32_t mask) {
    *status &= ~mask;
}

char sc2kc_table[KB_RELEASE_BASE] = {
    [0x00] = KB_KC_ERROR,
    [0x01] = 27,
    [0x02] = '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8,
    [0x0F] = 9, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
    [0x1C] = 13,
    [0x1D] = 2,
    [0x1E] = 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', 39,
    [0x29] '`',
    [0x2A] = KB_KC_LSHIFT,
    [0x2B] = '\\',
    [0x2C] = 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', KB_KC_RSHIFT,
    [0x37] = KB_KC_KEYPAD,
    [0x38] = KB_KC_LALT,
    [0x39] = ' ',
    [0x3A] = KB_KC_CAPSLOCK,
    [0x3B] = KB_KC_F(1), KB_KC_F(2), KB_KC_F(3), KB_KC_F(4), KB_KC_F(5),
    KB_KC_F(6), KB_KC_F(7), KB_KC_F(8), KB_KC_F(9), KB_KC_F(10),
    [0x45] = KB_KC_NUMLOCK,
    [0x46] = KB_KC_SCROLLLOCK,
    [0x47] = KB_KC_HOME, KB_KC_UP, KB_KC_PGUP,
    [0x4A] = KB_KC_MINUS,
    [0x4B] = KB_KC_LEFT, '5', KB_KC_RIGHT, '+',
    [0x4F] = KB_KC_END, KB_KC_DOWN, KB_KC_PGDN,
    [0x52] = KB_KC_INS, KB_KC_DEL,
    [0x54] = KB_KC_SYSRQ,
    [0x55] = KB_KC_F(11),
    [0x56] = KB_KC_UNLABELLED,
    [0x57] = KB_KC_F(11), KB_KC_F(12),
    [0x59 ... (KB_RELEASE_BASE - 1)] = KB_KC_NULL,
};

static char sc2kc(uint8_t sc) {
    int idx = (int)sc;
    return sc2kc_table[idx];
}


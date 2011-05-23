#include <debug.h>
#include <isr.h>
#include <idt.h>
#include <vga.h>
#include <mm.h>
#include <sched.h>
#include <i386.h>

const char* exp_name[] = {
	"Divide Error",
	"Debug Interrupt",
	"NMI Interrupt",
	"Breakpoint",
	"Interrupt on overflow",
	"BOUND range exceeded",
	"Invalid Opcode",
	"Device not available",
	"Double fault",
	"Coprocessor Segment Overrun",
	"Invalid TSS",
	"Segment not present",
	"Stack exception",
	"General protection fault",
	"Page fault",
	"Reserved",
	"Floating point exception",
	"Alignment check"
};

static void print_regs(const task_state_t *st);
static void print_stack(uint32_t *stack);
static void print_backtrace(const task_state_t *st);
static void print_int(uint32_t index, uint32_t error_code);
static void get_call_info(uint32_t ret_addr, uint32_t *at, uint32_t
    *func_addr);

#define DEBUG_KP_COL 30
#define DEBUG_KP_ROW 0
#define DEBUG_KP_ATTR 0x0F
bool in_panic = FALSE;
void debug_kernelpanic(const task_state_t *st, uint32_t index, uint32_t
    error_code) {
	/* No permite panics anidados */
	if (in_panic) while(1) hlt();
	in_panic = TRUE;

    vga_cls();
    vga_printf(DEBUG_KP_ROW, DEBUG_KP_COL, "Kernel Panic!",
        DEBUG_KP_ATTR);

    print_regs(st);
    print_stack((uint32_t *)st->esp);
    print_backtrace(st);
    print_int(index, error_code);
}

#define DEBUG_KP_REGS_COL 60
#define DEBUG_KP_REGS_INITROW 0
static void print_regs(const task_state_t *st) {
    char *regs[] = { "eax: %x", "ebx: %x", "ecx: %x", "edx: %x", "esi: %x",
        "edi: %x", "ebp: %x", "ss:  %x", "esp: %x", "cs:  %x", "eip: %x",
        "efl: %x", "tr: %x", "cr2: %x", "cr3: %x", "pid: %x" };
    uint32_t regs_values[] = { st->eax, st->ebx, st->ecx,
        st->edx, st->esi, st->edi, st->ebp, st->org_ss, st->esp, st->cs, st->eip,
        st->eflags, rtr(), rcr2(), rcr3(), 0x0 };

    uint16_t regs_row = DEBUG_KP_REGS_INITROW;
    int i;
    for (i = 0; i < (sizeof(regs)/sizeof(char *)); i++)
        vga_printf(regs_row++, DEBUG_KP_REGS_COL, regs[i], DEBUG_KP_ATTR,
            regs_values[i]);
}

#define DEBUG_KP_STACK_ROW 1
#define DEBUG_KP_STACK_COL 2
#define DEBUG_KP_STACK_PPR 4        // Posiciones por fila
#define DEBUG_KP_STACK_ROWS 13
static void print_stack(uint32_t *stack) {
    vga_printf(DEBUG_KP_STACK_ROW, DEBUG_KP_STACK_COL, "Stack:",
         DEBUG_KP_ATTR);

    const uint32_t *addr;
    uint16_t stack_row = DEBUG_KP_STACK_ROW + 1;
    for (addr = stack; addr < stack + DEBUG_KP_STACK_PPR*DEBUG_KP_STACK_ROWS;
        addr += DEBUG_KP_STACK_PPR) {
        vga_printf(stack_row++, DEBUG_KP_STACK_COL, "%x: %x %x %x %x",
            DEBUG_KP_ATTR, (uint32_t)addr, *(addr), *(addr + 1),
            *(addr + 2), *(addr + 3));
    }
}

#define DEBUG_KP_BT_ROW DEBUG_KP_STACK_ROW + 1 + \
    DEBUG_KP_STACK_ROWS + 1
#define DEBUG_KP_BT_COL DEBUG_KP_STACK_COL
#define DEBUG_KP_BT_ROWS 6
static void print_backtrace(const task_state_t *st) {
    uint16_t bt_row = DEBUG_KP_BT_ROW;
    vga_printf(bt_row++, DEBUG_KP_BT_COL, "Backtrace: Current: %x",
        DEBUG_KP_ATTR, st->eip);

    uint32_t *ebp = (uint32_t *)st->ebp;
    uint32_t ret_addr = *(ebp + 1);
    int i;
    for (i = 0; i < DEBUG_KP_BT_ROWS; i++) {
        uint32_t *first_parm = ebp + 2;

        uint32_t at = 0x0;
        uint32_t func_addr = 0x0;
        get_call_info(ret_addr, &at, &func_addr);

        if ((uint32_t *)at == &debug_backtrace_limit)
            break;

        vga_printf(bt_row++, DEBUG_KP_BT_COL, "%x: CALL %x "
            "(%x, %x, %x, %x)", DEBUG_KP_ATTR, at, func_addr,
            *first_parm, *(first_parm + 1), *(first_parm + 2),
            *(first_parm + 3));

        ebp = (uint32_t *)(*ebp);
        ret_addr = *(ebp + 1);
    }
}

#define DEBUG_KP_INT_ROW (VGA_ROWS - 1)
#define DEBUG_KP_INT_COL 0
#define DEBUG_KP_INT_DESC_COL 60
static void print_int(uint32_t index, uint32_t error_code) {
    vga_printf(DEBUG_KP_INT_ROW, DEBUG_KP_INT_COL, "INT %x err: %x ",
        DEBUG_KP_ATTR, index, error_code);

    if (index < sizeof(exp_name)/sizeof(char *))
        vga_printf(DEBUG_KP_INT_ROW, DEBUG_KP_INT_DESC_COL, exp_name[index],
            DEBUG_KP_ATTR);

}


static void get_call_info(uint32_t ret_addr, uint32_t *at, uint32_t
    *func_addr) {
    uint8_t *at_ptr = (uint8_t *)ret_addr - 6;
    // Call near, absolute indirect. Address given in m32.
    if (*at_ptr == 0xFF) {
        *at = (uint32_t)at_ptr;
        *func_addr = 0x0;
    }
    else {
        at_ptr++;
        // Call near, relative to next instruction. 32-bit displacement.
        if (*at_ptr == 0xE8) {
            *at = (uint32_t)at_ptr;
            
            int disp = *((int *)(at_ptr + 1));
            *func_addr = ret_addr + disp;
        }
        else {
        // Call near, relative to next instruction. 16-bit displacement.
            at_ptr += 2;
            if (*at_ptr == 0xE8) {
                *at = (uint32_t)at_ptr;
                *func_addr = 0x0;
            }
        }
    }
}
                

void debug_init(void) {
    // __asm__ __volatile__("mov $0x12345678, %eax; int $0x80");
}

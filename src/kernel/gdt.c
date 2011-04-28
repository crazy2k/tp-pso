#include <tipos.h>
#include <gdt.h>

#define COMMON_FLAGS (GDT_DESC_G | GDT_DESC_DB | GDT_DESC_P | GDT_DESC_S)

uint64_t gdt[] __attribute__((aligned(0x8))) = {

    [GDT_INDEX_NULL] = GDT_NULL,

    [GDT_INDEX_KERNEL_CS] = GDT_DESC_BASE(0x0) | GDT_DESC_LIMIT(0xFFFFF) |
    GDT_DESC_DPL(0x0) | GDT_DESC_TYPE(GDT_F_CODE_R) | COMMON_FLAGS,

    [GDT_INDEX_KERNEL_DS] = GDT_DESC_BASE(0x0) | GDT_DESC_LIMIT(0xFFFFF) |
    GDT_DESC_DPL(0x0) | GDT_DESC_TYPE(GDT_F_DATA_W) | COMMON_FLAGS,

    [GDT_INDEX_USER_CS] = GDT_DESC_BASE(0x0) | GDT_DESC_LIMIT(0xFFFFF) |
    GDT_DESC_DPL(0x3) | GDT_DESC_TYPE(GDT_F_CODE_R) | COMMON_FLAGS,

    [GDT_INDEX_USER_DS] = GDT_DESC_BASE(0x0) | GDT_DESC_LIMIT(0xFFFFF) |
    GDT_DESC_DPL(0x3) | GDT_DESC_TYPE(GDT_F_DATA_W) | COMMON_FLAGS,

    // El descriptor de la TSS se carga al momento de hacer el task switch
    [GDT_INDEX_TSS] = GDT_NULL,
};

gdtr_t gdtr = { .size = sizeof(gdt) - 1, .addr = &gdt };


void gdt_init(void) {

}

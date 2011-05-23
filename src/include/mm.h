#ifndef __MM_H__
#define __MM_H__

#include <tipos.h>

#define MM_ATTR_P     0x001 // Present
#define MM_ATTR_RW    0x002 // Read/Write
#define MM_ATTR_RW_R  0x000 //
#define MM_ATTR_RW_W  0x002 //
#define MM_ATTR_US    0x004 // User/Supervisor
#define MM_ATTR_US_U  0x004 //
#define MM_ATTR_US_S  0x000 //
#define MM_ATTR_WT    0x008 // Wrtie-Through
#define MM_ATTR_CD    0x010 // Cache Disabled
#define MM_ATTR_A     0x020 // Accessed
#define MM_ATTR_D     0x040 // Dirty (for Pages)
#define MM_ATTR_AVL   0x040 // Available (for Directory)
#define MM_ATTR_PAT   0x080 // Page Table Attribute Index (for Pages)
#define MM_ATTR_SZ_4K 0x000 // Page Size (for Directory)
#define MM_ATTR_SZ_4M 0x080 // Page Size (for Directory)
#define MM_ATTR_G     0x100 // Global (ignored for Directory)

#define MM_ATTR_USR   0xE00 // bits for kernel

/* Control Register flags */
#define CR0_PE    0x00000001    // Protection Enable
#define CR0_MP    0x00000002    // Monitor coProcessor
#define CR0_EM    0x00000004    // Emulation
#define CR0_TS    0x00000008    // Task Switched
#define CR0_ET    0x00000010    // Extension Type
#define CR0_NE    0x00000020    // Numeric Errror
#define CR0_WP    0x00010000    // Write Protect
#define CR0_AM    0x00040000    // Alignment Mask
#define CR0_NW    0x20000000    // Not Writethrough
#define CR0_CD    0x40000000    // Cache Disable
#define CR0_PG    0x80000000    // Paging

#define CR4_PCE    0x00000100    // Performance counter enable
#define CR4_MCE    0x00000040    // Machine Check Enable
#define CR4_PSE    0x00000010    // Page Size Extensions
#define CR4_DE     0x00000008    // Debugging Extensions
#define CR4_TSD    0x00000004    // Time Stamp Disable
#define CR4_PVI    0x00000002    // Protected-Mode Virtual Interrupts
#define CR4_VME    0x00000001    // V86 Mode Extensions

typedef struct str_mm_page {
    uint_32 attr:12;
    uint_32 base:20;
}  __attribute__((__packed__, aligned (4))) mm_page;

#define make_mm_entry(base, attr) (mm_page){(uint_32)(attr), (uint_32)(base)}
#define make_mm_entry_addr(addr, attr) (mm_page){(uint_32)(attr), (uint_32)(addr) >> 12}


#define PD_MASK __12_31_BITS__

#define PDI(laddr) (((uint32_t)(laddr) & __22_31_BITS__) >> 22)
#define PTI(laddr) (((uint32_t)(laddr) & __12_21_BITS__) >> 12)

#define PDE_PT_BASE(addr) ((uint32_t)(addr) & __12_31_BITS__)
#define PDE_PS (((uint32_t) 1) << 7)
#define PDE_A (((uint32_t) 1) << 5)
#define PDE_PCD (((uint32_t) 1) << 4)
#define PDE_PWT (((uint32_t) 1) << 3)
#define PDE_US (((uint32_t) 1) << 2)
#define PDE_RW (((uint32_t) 1) << 1)
#define PDE_P (((uint32_t) 1) << 0)

// page_t Table Entry

#define PTE_PAGE_BASE(dir) ((uint32_t)(dir) & __12_31_BITS__)
#define PTE_G (((uint32_t) 1) << 8)
#define PTE_PAT (((uint32_t) 1) << 7)
#define PTE_D (((uint32_t) 1) << 6)
#define PTE_A PDE_A
#define PTE_PCD PDE_PCD
#define PTE_PWT PDE_PWT
#define PTE_US PDE_US
#define PTE_RW PDE_RW
#define PTE_P PDE_P


#define PHADDR_TO_PAGE(phaddr) (((page_t *)FIRST_FREE_KERNEL_PAGE) + ((uint32_t)phaddr/PAGE_SIZE))
#define PAGE_TO_PHADDR(page) ((void*) ((page - (page_t*)FIRST_FREE_KERNEL_PAGE) * PAGE_SIZE) )


#define PAGE_SIZE 0x1000ul
#define PAGE_4MB_SIZE 0x400000ul
#define PAGE_MASK 0xFFFFF000
#define ALIGN_TO_PAGE_START(phaddr) ((void*)(PAGE_MASK & (uint32_t)phaddr))

#define ALIGN_TO_PAGE(addr, ceil) ALIGN_TO_N(addr, 0xFFFFF000, 0x1000, ceil)
#define ALIGN_TO_N(addr, mask, n, ceil) ({ uint32_t __addr = (uint32_t)(addr); \
    uint32_t __mask = (uint32_t)(mask); \
    uint32_t __n = (uint32_t)(n); \
    if ((ceil) && (__addr & ~__mask)) __addr += __n; \
    (void*) (__addr & __mask); })


#define FIRST_FREE_KERNEL_PAGE ((void*)0x100000)
#define LAST_POSIBLE_PAGE 0x100000
#define KERNEL_MEMORY_START ((void*)0x0)
#define KERNEL_MEMORY_LIMIT ((void*)0x400000)
#define KERNEL_STACK (0xA0000 - PAGE_SIZE)

#define TEST_WORD 0xF0F0F0F0


extern uint32_t kernel_pd[];

void mm_init(void);
void* mm_mem_alloc();
void* mm_mem_kalloc();
void mm_mem_free(void* page);

/* Manejador de directorios de página */
mm_page* mm_dir_new(void);
void mm_dir_free(mm_page* d);

void* new_user_page(uint32_t pd[], void* vaddr);

/* Syscalls */
// void* palloc(void);

#endif

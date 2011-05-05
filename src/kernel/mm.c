#include <mm.h>
#include <utils.h>
#include <i386.h>


static void free_pages_setup();
static void initialize_pd(uint32_t pd[]);
static void activate_pagination();
static void map_kernel_pages(uint32_t pd[], void *vstart, void *vstop);

static bool valid_phisical_page(void*);
static void link_pages(page_t*, page_t*);
static void add_page_to_list(page_t* head, page_t* new);

static void return_page(page_t**, page_t*);
static page_t *reserve_page(page_t**, page_t*);
static void reserve_pages(page_t**, void*, void*);
static page_t* take_free_page(page_t** page_list_ptr);
static page_t* take_free_kernel_page();

static uint32_t* get_pte(uint32_t pd[], void* vaddr);
static void *new_page_table(uint32_t pd[], void* vaddr);


page_t *free_user_pages = NULL, 
       *free_kernel_pages = NULL;

uint32_t kernel_pd[1024] __attribute__((aligned(PAGE_SIZE))) = {0};


void* mm_mem_alloc() {
}

void* mm_mem_kalloc() {
        page_t *page;
        if (page = take_free_kernel_page()) {
                return PAGE_TO_PHADDR(page);
        } else 
                return NULL;
}

void mm_mem_free(void* page) {
}

mm_page* mm_dir_new(void) {
	return NULL;
}

void mm_dir_free(mm_page* d) {
}


extern void* _end; // Puntero al fin del c'odigo del kernel.bin (definido por LD).

void mm_init(void) {
        free_pages_setup();
        initialize_pd(kernel_pd);
        activate_pagination();
}

static void free_pages_setup() {
        void *phaddr;

        free_kernel_pages = (page_t*)FIRST_FREE_KERNEL_PAGE;
        memset(free_kernel_pages, 0, sizeof(page_t));
        
        for (phaddr = KERNEL_MEMORY_START + PAGE_SIZE; phaddr < KERNEL_MEMORY_LIMIT; phaddr += PAGE_SIZE) {
                page_t* current = memset(PHADDR_TO_PAGE(phaddr), 0, sizeof(page_t));
                link_pages(current, PHADDR_TO_PAGE(current) - 1);
        }
        link_pages(PHADDR_TO_PAGE(phaddr) - 1, free_kernel_pages);

        free_user_pages = PHADDR_TO_PAGE(KERNEL_MEMORY_LIMIT);
        memset(free_user_pages, 0, sizeof(page_t));

        if (!valid_phisical_page(KERNEL_MEMORY_LIMIT)) custom_kpanic_msg("No hay suficiente memoria física disponible para ejecutar el SO");

        for (phaddr = KERNEL_MEMORY_LIMIT + PAGE_SIZE; valid_phisical_page(phaddr) && phaddr != NULL; phaddr += PAGE_SIZE) {
                page_t* current = memset(PHADDR_TO_PAGE(phaddr), 0, sizeof(page_t));
                link_pages(current, PHADDR_TO_PAGE(current) - 1);
        }
        page_t *last_used_page = PHADDR_TO_PAGE(phaddr) - 1;
        link_pages(last_used_page, free_user_pages);

        reserve_pages(&free_kernel_pages, KERNEL_MEMORY_START, last_used_page);
}

static void initialize_pd(uint32_t pd[]) {
        void* vaddr;

        for (vaddr = KERNEL_MEMORY_START + PAGE_SIZE; vaddr < KERNEL_MEMORY_LIMIT; vaddr += PAGE_4MB_SIZE) {
                uint32_t* table = new_page_table(kernel_pd, vaddr);
                pd[PDI(vaddr)] = PDE_PT_BASE(table) | PDE_P | PDE_PWT | PDE_RW;
        }
        map_kernel_pages(kernel_pd, KERNEL_MEMORY_START, KERNEL_MEMORY_LIMIT);
}

static void activate_pagination() {
        lcr3((uint32_t)kernel_pd);
        uint32_t new_cr0 = rcr0() | CR0_PG;
        lcr0(new_cr0);
}

static bool valid_phisical_page(void* phaddr) {
        uint32_t* test_addr = ALIGN_TO_PAGE_START(phaddr);
        *test_addr = TEST_WORD;

        return *test_addr == TEST_WORD;
}


static void link_pages(page_t* first, page_t* second)  { 
        first->next = second;
        second->prev = first;
}

static void add_page_to_list(page_t* head, page_t* new) {
    link_pages(new, head->next);
    link_pages(head, new);
}


static void return_page(page_t** page_list_ptr, page_t* reserved) {
        reserved->count--;

        if (reserved->count == 0) {
                if (*page_list_ptr)
                        add_page_to_list(*page_list_ptr, reserved);
                else
                        *page_list_ptr = reserved->next = reserved->prev = reserved;

        }
}

static page_t *reserve_page(page_t** page_list_ptr, page_t* page) {
        if (page->next && page->prev)
                link_pages(page->prev, page->next);

        if (*page_list_ptr == page) {
                *page_list_ptr = (page->next != page) ? page->next:  NULL;
        }

        page->count++;
        page->next = page->prev = NULL;

        return page;
}

static void reserve_pages(page_t** page_list_ptr, void* phaddr, void* limit) {
        for (phaddr = ALIGN_TO_PAGE_START(phaddr); phaddr < limit; phaddr += PAGE_SIZE) {
                reserve_page(page_list_ptr, PHADDR_TO_PAGE(phaddr));
        }
}

static page_t* take_free_kernel_page() {
        return take_free_page(&free_kernel_pages);
}

static page_t* take_free_page(page_t** page_list_ptr) {
        if (!*page_list_ptr)
                return NULL;
        else
                return reserve_page(page_list_ptr, (*page_list_ptr)->next);
}


static void map_kernel_pages(uint32_t pd[], void *vstart, void *vstop) {
        void *vaddr;
        for (vaddr = vstart; vaddr < vstop; vaddr += PAGE_SIZE) {

                *get_pte(pd, vaddr) = PTE_PAGE_BASE(vaddr) | PTE_G | PTE_PWT |
                        PTE_RW | PTE_P;
        }
}

static uint32_t* get_pte(uint32_t pd[], void* vaddr) {
        uint32_t pde = pd[PDI(vaddr)];

        //Si la tabla de paginas no esta disponible, retornar NULL
        if (!(pde & PDE_P))
                return NULL;

        uint32_t *pt = (void *)PDE_PT_BASE(pde);
        return &pt[PTI(vaddr)];
}

// Mapea una pagina fisica nueva para una tabla de paginas de page_dir
static void *new_page_table(uint32_t pd[], void* vaddr) {
        void *page_va = mm_mem_kalloc(); 

        pd[PDI(vaddr)] = PDE_PT_BASE(page_va) | PDE_P | PDE_PWT | PDE_US | PDE_RW;
        memset(page_va, 0, PAGE_SIZE);

        return page_va;
}

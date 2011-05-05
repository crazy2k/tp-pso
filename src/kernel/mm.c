#include <mm.h>
#include <utils.h>
#include <i386.h>


typedef struct page_t page_t;

struct page_t {
    int count;
    page_t *next;
    page_t *prev;
};


static void free_pages_list_setup(void);
static uint32_t*  initialize_pd(uint32_t pd[]);
static void activate_pagination(void);
static void map_kernel_pages(uint32_t pd[], void *vstart, void *vstop);

static bool valid_phisical_page(void*);
static void link_pages(page_t*, page_t*);
static void add_page_to_list(page_t* head, page_t* new);

static void return_page(page_t**, page_t*);
static page_t *reserve_page(page_t**, page_t*);
static void reserve_pages(page_t**, void*, void*);
static page_t* take_free_page(page_t** page_list_ptr);
static page_t* take_free_kernel_page(void);

static uint32_t* get_pte(uint32_t pd[], void* vaddr);
static void *new_page_table(uint32_t pd[], void* vaddr);

static void* new_user_page(uint32_t pd[], void* vaddr);
static void free_user_page(uint32_t pd[], void* vaddr);
static void* seek_unused_vaddr(uint32_t pd[]);


page_t *free_user_pages = NULL, 
       *free_kernel_pages = NULL;

uint32_t kernel_pd[1024] __attribute__((aligned(PAGE_SIZE))) = {0};


void* mm_mem_alloc() {
        uint32_t *pd = mm_current_pd();
        void *vaddr = seek_unused_vaddr(pd);

        if (vaddr) {
                return new_user_page(pd, vaddr);
        } else 
                return NULL;

}

void* mm_mem_kalloc() {
        page_t *page = take_free_kernel_page();

        if (page) {
                return PAGE_TO_PHADDR(page);
        } else 
                return NULL;
}

void mm_mem_free(void* vaddr) {
        if (vaddr < KERNEL_MEMORY_LIMIT) {
                return_page(&free_kernel_pages, PHADDR_TO_PAGE(vaddr));
        } else {
                free_user_page(mm_current_pd(), vaddr);
        }
}

mm_page* mm_dir_new(void) {
	uint32_t *pd = mm_mem_kalloc();
        return (mm_page*) initialize_pd(pd);
}

void mm_dir_free(mm_page* mm_page) {
	uint32_t *pd = (uint32_t*)mm_page;
        int i, j;
             
        for (i = 0; i < 1024; i++) {
                if (pd[i] & PDE_P) {
                        void *vaddr;
                        for (vaddr = (void*)(PAGE_4MB_SIZE * i), j = 0; j < 1024; vaddr += PAGE_SIZE) {
                                uint32_t *pte = get_pte(pd, vaddr);
                                if (*pte & PTE_P)
                                        mm_mem_free(vaddr);
                                 
                        }
                }
        }
}


extern void* _end; // Puntero al fin del c'odigo del kernel.bin (definido por LD).

uint32_t *mm_current_pd(void) {
        return (uint32_t*)rcr3();
}

void mm_init(void) {
        free_pages_list_setup();
        initialize_pd(kernel_pd);
        activate_pagination();
}

static void free_pages_list_setup(void) {
        void *phaddr;

        free_kernel_pages = (page_t*)FIRST_FREE_KERNEL_PAGE;
        memset(free_kernel_pages, 0, sizeof(page_t));
        
        for (phaddr = KERNEL_MEMORY_START + PAGE_SIZE; phaddr < KERNEL_MEMORY_LIMIT; 
                phaddr += PAGE_SIZE) {

                page_t* current = memset(PHADDR_TO_PAGE(phaddr), 0, sizeof(page_t));
                link_pages(current, PHADDR_TO_PAGE(current) - 1);
        }
        link_pages(PHADDR_TO_PAGE(phaddr) - 1, free_kernel_pages);

        free_user_pages = PHADDR_TO_PAGE(KERNEL_MEMORY_LIMIT);
        memset(free_user_pages, 0, sizeof(page_t));

        if (!valid_phisical_page(KERNEL_MEMORY_LIMIT)) {
                custom_kpanic_msg("No hay suficiente memoria física "
                        "disponible para ejecutar el SO");
        }

        for (phaddr = KERNEL_MEMORY_LIMIT + PAGE_SIZE; 
                valid_phisical_page(phaddr) && phaddr != NULL; phaddr += PAGE_SIZE) {

                page_t* current = memset(PHADDR_TO_PAGE(phaddr), 0, sizeof(page_t));
                link_pages(current, PHADDR_TO_PAGE(current) - 1);
        }
        page_t *last_used_page = PHADDR_TO_PAGE(phaddr) - 1;
        link_pages(last_used_page, free_user_pages);

        reserve_pages(&free_kernel_pages, KERNEL_MEMORY_START, last_used_page);
}

static uint32_t* initialize_pd(uint32_t pd[]) {
        void* vaddr;

        for (vaddr = KERNEL_MEMORY_START + PAGE_SIZE; vaddr < KERNEL_MEMORY_LIMIT; 
                vaddr += PAGE_4MB_SIZE) {

                uint32_t* table = new_page_table(kernel_pd, vaddr);
                pd[PDI(vaddr)] = PDE_PT_BASE(table) | PDE_P | PDE_PWT | PDE_RW;
        }
        map_kernel_pages(kernel_pd, KERNEL_MEMORY_START, KERNEL_MEMORY_LIMIT);

        return pd;
}

static void activate_pagination(void) {
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
        if (reserved->count > 0) {

                if (--reserved->count == 0) {

                        if (*page_list_ptr)
                                add_page_to_list(*page_list_ptr, reserved);
                        else
                                *page_list_ptr = reserved->next = reserved->prev = reserved;

                }
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

                *get_pte(pd, vaddr) = PTE_PAGE_BASE(vaddr) | PTE_PWT | PTE_RW | PTE_P;
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

// Mapea una pagina fisica nueva para la direccion virtual pasada por parametro 
static void* new_user_page(uint32_t pd[], void* vaddr) {
        // Si no hay mas memoria retornar NULL
        if (!free_user_pages) 
                return NULL;

        vaddr = ALIGN_TO_PAGE_START(vaddr);

        // Si la tabla de paginas no estaba presente mapearla
        if (!(pd[PDI(vaddr)] & PDE_P))
                new_page_table(pd, vaddr);

        page_t *page = take_free_page(&free_user_pages);
        uint32_t *pte = get_pte(pd, vaddr);

        // Si la direccion ya fúe mapeada retornar NULL
        if (*pte & PDE_P) {
                custom_kpanic_msg("La direccion virtual que se intentaba asignar"
                        " ya estaba mapeada en el directorio");
        }

        *pte = PTE_PAGE_BASE(PAGE_TO_PHADDR(page)) | PDE_P | PDE_PWT | PDE_US | PDE_RW;

        return vaddr; 
}

// Retorna la pagina fisica correspondiente a la direccion virtual a la lista
// de paginas libres, y la borra la entry en la tabla de paginas
static void free_user_page(uint32_t pd[], void* vaddr) {
        uint32_t *pte = get_pte(pd, vaddr);
        page_t *page = PHADDR_TO_PAGE(PTE_PAGE_BASE(*pte));

        return_page(&free_kernel_pages, page);
        *pte = 0;
}

static void* seek_unused_vaddr(uint32_t pd[]) {
        void *vaddr;
        for (vaddr = KERNEL_MEMORY_LIMIT; vaddr != NULL; vaddr += PAGE_SIZE) {
                uint32_t *pte = get_pte(kernel_pd, vaddr);
                if ((*pte & PTE_P) == 0)
                        return (void*)PTE_PAGE_BASE(*pte);
        }

        return NULL;
}

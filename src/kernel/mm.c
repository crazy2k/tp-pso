#include <mm.h>
#include <errors.h>
#include <debug.h>
#include <utils.h>
#include <i386.h>
#include <swap.h>

#define IS_ASSIGNED_PAGE(pte) ((((pte) & PTE_AVL_BITS) == PTE_ASSIGNED_PAGE) \
    && ((pte) & PTE_P))
#define IS_REQUESTED_PAGE(pte) ((((pte) & PTE_AVL_BITS) == PTE_REQUESTED_PAGE) \
    && !((pte) & PTE_P))
#define IS_SHARED_PAGE(pte) ((((pte) & PTE_AVL_BITS) == PTE_SHARED_PAGE) \
    && ((pte) & PTE_P))
#define IS_COW_PAGE(pte) ((((pte) & PTE_AVL_BITS) == PTE_COW_PAGE) \
    && ((pte) & PTE_P) && !((pte) & PDE_RW))
#define IS_SWAPPED_PAGE(pte) ((((pte) & PTE_AVL_BITS) == PTE_SWAPPED_PAGE) \
    && !((pte) & PTE_P))

#define MARK_AS_COW(pte) ((CLEAR_PTE_AVL_BITS(pte) | PTE_COW_PAGE) & ~PTE_RW)
#define MARK_AS_SHARED(pte) (CLEAR_PTE_AVL_BITS(pte) | PTE_SHARED_PAGE)
#define MARK_AS_SWAPPED(pte) (CLEAR_PTE_AVL_BITS(pte) | PTE_SWAPPED_PAGE)
#define CLEAR_PTE_AVL_BITS(pte) ((pte) & ~PTE_AVL_BITS)

#define PHADDR_TO_PAGE(phaddr) (((page_t *)FIRST_FREE_KERNEL_PAGE) + ((uint32_t)phaddr/PAGE_SIZE))
#define PAGE_TO_PHADDR(page) ((void*) ((page - (page_t*)FIRST_FREE_KERNEL_PAGE) * PAGE_SIZE) )

typedef struct page_t page_t;

/*Podria reducirse el tamaño eliminando prev y usando un contador de 16 bits*/
struct page_t {
    int count;
    page_t *next;
    page_t *prev;
} __attribute__((__packed__));



/* Maybe Shared Resource (Aunque cada proceso tiene su propia pd y sus propias tablas)*/
uint32_t kernel_pd[PD_ENTRIES] __attribute__((aligned(PAGE_SIZE))) = {0};


/* Shared Resource */
static page_t *free_user_pages = NULL,
       /* Shared Resource */
       *free_kernel_pages = NULL;



static uint32_t *current_pd(void);

static void free_pages_list_setup(void);
static void user_pages_list_setup(void);
static void kernel_pages_list_setup(void);
static uint32_t*  initialize_pd(uint32_t pd[]);
static void activate_pagination(void);
static void map_kernel_pages(uint32_t pd[], void *vstart, void *vstop);

static void* set_page_as_requested(void *vaddr);

static bool valid_physical_page(void*);
static void add_page_to_list(page_t* head, page_t* new);

static void return_page(page_t**, page_t*);
static void return_kernel_page(page_t*);
static page_t *reserve_page(page_t**, page_t*);
static void reserve_pages(page_t**, void*, void*);
static page_t* take_free_page(page_t** page_list_ptr);

static uint32_t* get_pte(uint32_t pd[], void* vaddr);
static uint32_t* get_pte_table_alloc(uint32_t pd[], void* vaddr);
static void *new_page_table(uint32_t pd[], void* vaddr);

static uint32_t clone_pte(uint32_t pd[], void* from, void* avl_addr);

static void free_user_page(uint32_t pd[], void* vaddr);
static void* seek_unused_vaddr(uint32_t pd[]);
static void print_totals(uint32_t pd[]);

static void *last_unused_vaddr = KERNEL_MEMORY_LIMIT;

void mm_init(void) {
    free_pages_list_setup();
    initialize_pd(kernel_pd);
    activate_pagination();
}

void* mm_mem_alloc() {
    uint32_t *pd = current_pd();
    void *vaddr = seek_unused_vaddr(pd);

    if (vaddr) {
        return new_user_page(pd, vaddr);
    } else
        return NULL;

}

void* mm_mem_kalloc() {
    page_t *page = take_free_page(&free_kernel_pages);

    if (page) {
        return PAGE_TO_PHADDR(page);
    } else
        return NULL;
}

void mm_mem_free(void* vaddr) {
    if (vaddr < KERNEL_MEMORY_LIMIT) {
        return_kernel_page(PHADDR_TO_PAGE(vaddr));
    } else {
        free_user_page(current_pd(), vaddr);
    }
}

uint32_t* mm_dir_new(void) {
	uint32_t *pd = mm_mem_kalloc();
    uint32_t *new_dir =(uint32_t *) initialize_pd(pd);
    return new_dir;
}

void mm_dir_free(uint32_t* pd) {
	uint32_t i, j;
    void *phaddr, *vaddr;

    phaddr = (void*) PDE_PT_BASE(pd[0]);
    return_kernel_page(PHADDR_TO_PAGE(phaddr));

    for (i = 1; i < PD_ENTRIES; i++) {
        if (pd[i] & PDE_P) {
            vaddr = (void*)(PAGE_4MB_SIZE * i);
            for (j = 0; j < PT_ENTRIES; j++, vaddr += PAGE_SIZE) {
                uint32_t *pte = get_pte(pd, vaddr);
                if (pte && (*pte & PTE_P))
                    free_user_page(pd, vaddr);
            }

            phaddr = (void*) PDE_PT_BASE(pd[i]);
            return_kernel_page(PHADDR_TO_PAGE(phaddr));
        }
    }

    for (i = 0; i < PD_ENTRIES; i++) { 
        phaddr = (void*) PDE_PT_BASE(pd[i]);
        if (pd[i] & PDE_P)
            return_kernel_page(PHADDR_TO_PAGE(phaddr));
    }

    return_kernel_page(PHADDR_TO_PAGE(pd));
}

void* mm_request_mem_alloc() {
    uint32_t *pd = current_pd();
    void *vaddr = seek_unused_vaddr(pd);
    debug_printf("mm_request_mem_alloc: vaddr: %x\n", vaddr);
    debug_printf("mm_request_mem_alloc: free_user_pages size: %d\n", SIZE(free_user_pages));
//    print_totals(pd);
    

    return set_page_as_requested(vaddr);
}

void* mm_load_requested_page(void* vaddr) {
    return new_user_page(current_pd(), vaddr);
}

int mm_load_cow_page(void* vaddr) {
    uint32_t *pd = current_pd();
    void *avl_addr = seek_unused_vaddr(pd);

    uint32_t *pte = get_pte(pd, vaddr);
    debug_printf("mm_load_cow_page: PTE: %x\n", *pte);
    if (*pte && IS_COW_PAGE(*pte)) {
        page_t *mapped_page = PHADDR_TO_PAGE(PTE_PAGE_BASE(*pte));
        debug_printf("mm_load_cow_page: mapped_page: count: %x, phaddr: %x\n",
            mapped_page->count, PAGE_TO_PHADDR(mapped_page));
        if (mapped_page->count > 1) {
            debug_printf("mm_load_cow_page: mapped_page->count > 1\n");
            *pte = clone_pte(pd, ALIGN_TO_PAGE_START(vaddr), avl_addr);
            return_page(&free_user_pages, mapped_page);
        } else {
            debug_printf("mm_load_cow_page: mapped_page->count <= 1\n");
            *pte = CLEAR_PTE_AVL_BITS(*pte) | PTE_RW;
        }

        debug_printf("mm_load_cow_page: new PTE: %x\n", *pte);

        return 0;
    } else
        return -1;
}

bool mm_is_cow_page(void* vaddr) {
    uint32_t *pte = get_pte(current_pd(), vaddr);

    return IS_COW_PAGE(*pte);
}

bool mm_is_requested_page(void* vaddr) {
    uint32_t *pte = get_pte(current_pd(), vaddr);

    return IS_REQUESTED_PAGE(*pte);
}

bool mm_is_swapped_page(void* vaddr) {
    uint32_t *pte = get_pte(current_pd(), vaddr);

    return IS_SWAPPED_PAGE(*pte);
}

uint32_t *mm_clone_pd(uint32_t src_pd[]) {

    debug_printf("mm_clone_pd: start\n");

    // Creamos un nuevo PD vacio
    uint32_t *dest_pd = (uint32_t*)mm_dir_new();

    void *vaddr;
    for (vaddr = KERNEL_MEMORY_LIMIT; vaddr != NULL; vaddr += PAGE_4MB_SIZE) {
        // Si no hay ninguna tabla en el src, pasamos a los siguientes 4MB
        if (!(src_pd[PDI(vaddr)] & PDE_P))
            continue;

        // Recorremos la tabla del src
        uint32_t offset;
        for (offset = 0; offset < PAGE_4MB_SIZE; offset += PAGE_SIZE) {
            uint32_t *src_pte = get_pte(src_pd, vaddr + offset);

            // La PTE puede estar marcada como:
            //  1. presente (asignada, COW, shared)
            //  2. no presente (pedida, simplemente invalida)

           // Si la PTE esta marcada como invalida pasamos a la siguiente
            if (!(*src_pte & PTE_P) && !(IS_REQUESTED_PAGE(*src_pte)))
                continue;

            uint32_t *dest_pte = get_pte_table_alloc(dest_pd, vaddr + offset);

            // Inicialmente copiamos la entrada (es lo que hay que hacer en
            // cualquier caso, excepto si esta asignada)
            *dest_pte = *src_pte;
            if (*src_pte & PTE_P) {
                // Si la pagina estaba asignada en el src, la marcamos COW en
                // el src y en el dest
                if (IS_ASSIGNED_PAGE(*src_pte)) {
                    *src_pte = *dest_pte = MARK_AS_COW(*src_pte);
                }
                // XXX: Es esto necesario?
                reserve_page(&free_user_pages,
                    PHADDR_TO_PAGE(PTE_PAGE_BASE(*src_pte)));
            }
        }
    }

    debug_printf("mm_clone_pd: end\n");
    return dest_pd;
}

int mm_share_page(void* vaddr) {
    vaddr = ALIGN_TO_PAGE_START(vaddr);
    uint32_t *pte = get_pte(current_pd(), vaddr);

    if (IS_REQUESTED_PAGE(*pte)) {
        debug_printf("mm_share_page: mm_load_requested_page\n");
        if (mm_load_requested_page(vaddr) == NULL)
            return -ENOMEM;
    }

    if (IS_COW_PAGE(*pte)) {
        debug_printf("mm_share_page: mm_load_cow_page\n");
        if (mm_load_cow_page(vaddr) == -1)
            return -EINVALID;
    }

    if (pte == NULL || !(*pte & PTE_P))
        return -EINVALID;
    else if (!(*pte & PTE_US))
        return -ENOPERM;
    else {
        debug_printf("mm_share_page: MARK_AS_SHARED()\n");
        *pte = MARK_AS_SHARED(*pte);
        return 0;
    }
}

extern void* _end; // Puntero al fin del codigo del kernel.bin (definido por LD).

uint32_t *current_pd(void) {
    return (uint32_t *)(PD_MASK & rcr3());
}

static void free_pages_list_setup(void) {

    kernel_pages_list_setup();
    user_pages_list_setup();
}


static void kernel_pages_list_setup(void) {
    void *phaddr;

    page_t *first = (page_t *)FIRST_FREE_KERNEL_PAGE;
    memset(first, 0, sizeof(page_t));
    APPEND(&free_kernel_pages, first);

    for (phaddr = KERNEL_MEMORY_START + PAGE_SIZE; phaddr < KERNEL_MEMORY_LIMIT;
        phaddr += PAGE_SIZE) {

        page_t *current = PHADDR_TO_PAGE(phaddr);
        memset(current, 0, sizeof(page_t));
        APPEND(&free_kernel_pages, current);
    }
}


static void user_pages_list_setup(void) {
    void *phaddr;

    page_t *first = PHADDR_TO_PAGE(KERNEL_MEMORY_LIMIT);
    memset(first, 0, sizeof(page_t));

    APPEND(&free_user_pages, first);
    uint32_t free_user_pages_count = 1;

    if (!valid_physical_page(KERNEL_MEMORY_LIMIT)) {
        custom_kpanic_msg("No hay suficiente memoria física "
            "disponible para ejecutar el SO");
    }

    for (phaddr = KERNEL_MEMORY_LIMIT + PAGE_SIZE;
        valid_physical_page(phaddr) && phaddr != NULL; phaddr += PAGE_SIZE) {

        page_t *current = PHADDR_TO_PAGE(phaddr);
        memset(current, 0, sizeof(page_t));
        APPEND(&free_user_pages, current);
        debug_printf("user_pages_list_setup: count: %d\n", ++free_user_pages_count);
    }

    reserve_pages(&free_kernel_pages, KERNEL_MEMORY_START,
        (void *)PHADDR_TO_PAGE(phaddr));
}

static uint32_t* initialize_pd(uint32_t pd[]) {
    void* vaddr;
    memset((void*)pd, 0, PAGE_SIZE);

    for (vaddr = KERNEL_MEMORY_START + PAGE_SIZE; vaddr < KERNEL_MEMORY_LIMIT;
        vaddr += PAGE_4MB_SIZE) {

        uint32_t* table = new_page_table(pd, vaddr);
        pd[PDI(vaddr)] = PDE_PT_BASE(table) | PDE_P | PDE_PWT | PDE_RW;
    }

    map_kernel_pages(pd, KERNEL_MEMORY_START, KERNEL_MEMORY_LIMIT);

    return pd;
}

static void activate_pagination(void) {
    lcr3((uint32_t)kernel_pd);
    uint32_t new_cr0 = rcr0() | CR0_PG;
    lcr0(new_cr0);
}

static uint32_t valid_pages_count = 0;

static bool valid_physical_page(void* phaddr) {
    volatile uint32_t* test_addr = ALIGN_TO_PAGE_START(phaddr);
    *test_addr = TEST_WORD;

    debug_printf("valid_physical_page: phaddr: %x\n", phaddr);
    valid_pages_count++;
    return *test_addr == TEST_WORD;
}

static void add_page_to_list(page_t* head, page_t* new) {
    LINK_NODES(new, head->next);
    LINK_NODES(head, new);
}


static void return_page(page_t** page_list_ptr, page_t* reserved) {
    debug_printf("return_page: count: %x, phaddr: %x\n", reserved->count,
        PAGE_TO_PHADDR(reserved));
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
        LINK_NODES(page->prev, page->next);

    if (*page_list_ptr == page) {
        *page_list_ptr = (page->next != page) ? page->next : NULL;
    }

    page->count++;
    page->next = page->prev = NULL;

    debug_printf("reserve_page: count: %x phaddr: %x\n",
        page->count, PAGE_TO_PHADDR(page));

    return page;
}

static void reserve_pages(page_t** page_list_ptr, void* phaddr, void* limit) {
    for (phaddr = ALIGN_TO_PAGE_START(phaddr); phaddr < limit; phaddr += PAGE_SIZE) {
        reserve_page(page_list_ptr, PHADDR_TO_PAGE(phaddr));
    }
}

static void return_kernel_page(page_t* reserved) {
    return_page(&free_kernel_pages, reserved);
}

static page_t* take_free_page(page_t** page_list_ptr) {
    if (!*page_list_ptr)
        return NULL;
    else
        return reserve_page(page_list_ptr, *page_list_ptr);
}


static void map_kernel_pages(uint32_t pd[], void *vstart, void *vstop) {
    void *vaddr;
    for (vaddr = vstart; vaddr < vstop; vaddr += PAGE_SIZE) {

        *get_pte(pd, vaddr) = PTE_PAGE_BASE(vaddr) | PTE_PWT | PTE_RW | PTE_P;
    }
}

static uint32_t* get_pte_table_alloc(uint32_t pd[], void* vaddr) {
    if (!(pd[PDI(vaddr)] & PDE_P))
        new_page_table(pd, vaddr);

    return get_pte(pd, vaddr);
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

static void* set_page_as_requested(void *vaddr) {
    uint32_t *pte = get_pte_table_alloc(current_pd(), vaddr);

    // Si la direccion ya fue mapeada retornar NULL
    if (*pte & PDE_P) {
        debug_printf("set_page_as_requested: page present\n");
        custom_kpanic_msg("La direccion virtual que se intentaba asignar"
            " ya estaba mapeada en el directorio");
    }


    *pte = PTE_REQUESTED_PAGE;

    return vaddr;
}

static void* get_victim_vaddr(uint32_t pd[]) {
    void *vaddr;
    for (vaddr = 0xC0000000 - PAGE_SIZE; vaddr != KERNEL_MEMORY_LIMIT; vaddr -= PAGE_SIZE) {
//    for (vaddr = KERNEL_MEMORY_LIMIT; vaddr != NULL; vaddr += PAGE_SIZE) {
        // Note that pte could be zero when there's no pte for this vaddr
        uint32_t *pte = get_pte(pd, vaddr);
        if (pte != NULL && *pte && IS_ASSIGNED_PAGE(*pte) && !IS_SWAPPED_PAGE(*pte)) {
            debug_printf("get_victim_vaddr: vaddr %x\n", vaddr);
            return vaddr;
        }
    }

    return NULL;
}


void mm_swap_page_in(void *vaddr) {
    debug_printf("mm_swap_page_in: vaddr %x\n", vaddr);

    uint32_t *pd = current_pd();
    uint32_t *pte = get_pte(pd, vaddr);
    if (IS_SWAPPED_PAGE(*pte)) {
        uint32_t id = PTE_NOT_PRESENT_SWAP_ID(*pte) >> 12;
        void *aligned_vaddr = new_user_page(pd, vaddr);
        swap_load(id, aligned_vaddr);
    }
}

void mm_swap_page_out() {
    debug_printf("mm_swap_page_out:\n");

    uint32_t *pd = current_pd(); 
    void *victim_vaddr = get_victim_vaddr(pd);
    uint32_t id = swap_unload(victim_vaddr);
    uint32_t *victim_pte = get_pte(pd, victim_vaddr);
    page_t *mapped_page = PHADDR_TO_PAGE(PTE_PAGE_BASE(*victim_pte));
    return_page(&free_user_pages, mapped_page);
    *victim_pte = 0;
    *victim_pte = MARK_AS_SWAPPED(*victim_pte) | (id << 12);
}

//uint32_t list_size(page_t *list_ptr) {
//    page_t *tip = list_ptr;
//    page_t *node = tip->next;
//    if (node == tip) {
//        return 1
//    }
//
//    int size = 1;
//    while (node != tip) {
//        size++;
//        node ->
//    }
//}
//    ({ \
//        typeof(list_ptr) _tip = (list_ptr); \
//        typeof(list_ptr) _node = (_tip)->next; \
//        int _size = 1; \
//        while (_node != _tip) { \
//            _size++; \
//            _node = _tip->next; \
//        } \
//        _size; \
//    })

// Mapea una pagina fisica nueva para la direccion virtual pasada por parametro
void* new_user_page(uint32_t pd[], void* vaddr) {
    debug_printf("new_user_page:\n");
    // Si no hay mas memoria retornar NULL
    if (SIZE(free_user_pages) < 10) {
        debug_printf("new_user_page: NO hay pagina libre");
        mm_swap_page_out();
    } else {
        debug_printf("new_user_page: Hay pagina libre");
    }

    vaddr = ALIGN_TO_PAGE_START(vaddr);

    page_t *page = take_free_page(&free_user_pages);
    debug_printf("new_user_page: page %x\n", page);
    uint32_t *pte = get_pte_table_alloc(pd, vaddr);

    // Si la direccion ya fue mapeada retornar NULL
    if (*pte & PDE_P) {
        custom_kpanic_msg("La direccion virtual que se intentaba asignar"
            " ya estaba mapeada en el directorio");
    }

    *pte = PTE_PAGE_BASE(PAGE_TO_PHADDR(page)) | PDE_P | PDE_PWT | PDE_US | PDE_RW;

    return vaddr;
}

static uint32_t clone_pte(uint32_t pd[], void* from, void* avl_addr) {
    debug_printf("clone_pte: before new_user_page: from %x, avl_addr: %x\n",
        from, avl_addr);
    avl_addr = new_user_page(pd, avl_addr);
    debug_printf("clone_pte: after new_user_page: from %x, avl_addr: %x\n",
        from, avl_addr);
    memcpy(avl_addr, from, PAGE_SIZE);

    int result = *get_pte(pd, avl_addr);

    *get_pte(pd, avl_addr) = 0;

    return result;
}

// Retorna la pagina fisica correspondiente a la direccion virtual a la lista
// de paginas libres, y la borra la entry en la tabla de paginas
static void free_user_page(uint32_t pd[], void* vaddr) {
    uint32_t *pte = get_pte(pd, vaddr);
    page_t *page = PHADDR_TO_PAGE(PTE_PAGE_BASE(*pte));

    return_page(&free_user_pages, page);
    *pte = 0;
}

static void* seek_unused_vaddr(uint32_t pd[]) {
    void *vaddr;
    for (vaddr = last_unused_vaddr; vaddr != NULL; vaddr += PAGE_SIZE) {
        uint32_t *pte = get_pte_table_alloc(pd, vaddr);
        debug_printf("seek_unused_vaddr: vaddr: %x, pte: %x\n", vaddr, *pte);
        debug_printf("seek_unused_vaddr: pte present: %x, "
            "pte requested: %x\n", (*pte & PTE_P), IS_REQUESTED_PAGE(*pte));
        if (!(*pte)) {
            last_unused_vaddr = vaddr;
            return vaddr;
        }
    }

    return NULL;
}
static void print_totals(uint32_t pd[]) {
    uint32_t present_count = 0;
    uint32_t swapped_count = 0;
    void *vaddr;
    for (vaddr = KERNEL_MEMORY_LIMIT; vaddr != NULL; vaddr += PAGE_SIZE) {
        uint32_t *pte = get_pte(pd, vaddr);
        if (pte != NULL) {
            if (*pte & PDE_P) {
                present_count++;
            }
            if (mm_is_swapped_page(vaddr)) {
                swapped_count++;
            }
        }
    }
    debug_printf("print_totals: present = %d, swapped = %d\n", present_count, swapped_count);
}

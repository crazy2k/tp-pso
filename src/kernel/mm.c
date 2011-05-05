#include <mm.h>
#include <utils.h>


static void free_pages_setup();
static void kernel_page_dir_setup();

static bool valid_phisical_page(void*);
static void link_pages(page_t*, page_t*);
static void add_page_to_list(page_t* head, page_t* new);

static void return_page(page_t**, page_t*);
static page_t *reserve_page(page_t**, page_t*);
static void reserve_pages(page_t**, void*, void*);


page_t *free_user_pages = NULL, 
       *free_kernel_pages = NULL;
uint32_t kernel_pd[1024] __attribute__((aligned(PAGE_SIZE))) = {0};


void* mm_mem_alloc() {
	return NULL;
}

void* mm_mem_kalloc() {
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
        kernel_page_dir_setup();
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

static void kernel_page_dir_setup() {
        
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

static page_t* take_free_page(page_t** page_list_ptr) {
        if (!*page_list_ptr) 
                custom_kpanic_msg("Memoria física disponible agotada");

        return reserve_page(page_list_ptr, (*page_list_ptr)->next);
}

#include <mm.h>
#include <utils.h>


static bool valid_phisical_page(uint32_t);
static void join_pages(page_t*, page_t*);


uint32_t kernel_pd[1024] __attribute__((aligned(PAGE_SIZE))) = {0};
page_t* page_list = NULL;


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
        uint32_t page;

        //if (!valid_phisical_page(FIRST_FREE_PAGE)) kpanic();
        page_list = (page_t*)FIRST_FREE_PAGE_ADDR;
        memset(page_list, 0, sizeof(page_t));
        
        for (page = FIRST_FREE_PAGE + 1; page < LAST_POSIBLE_PAGE && valid_phisical_page(page); page++) {
                page_t* current =  memset(PAGE_NUM_TO_PAGE_T(page), 0, sizeof(page_t));
                join_pages(current, PAGE_NUM_TO_PAGE_T(page - 1));
        }

        join_pages(PAGE_NUM_TO_PAGE_T(page - 1), page_list);
        	
}

static bool valid_phisical_page(uint32_t page_number) {
        uint32_t* pharrd = (uint32_t*)PAGE_NUM_TO_PHADDR(page_number * PAGE_SIZE);
        *pharrd = TEST_WORD;

        return *pharrd == TEST_WORD;
}


static void join_pages(page_t* first, page_t* second)  { 
        first->next = second;
        second->prev = first;
}

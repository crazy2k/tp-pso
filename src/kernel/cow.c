#include <cow.h>
#include <mm.h>
#include <utils.h>


#define HASH_TABLE_LEGTH 256
#define FREE_NODES_LEGTH 256

typedef struct cow_t cow_t;
struct cow_t {
    uint32_t phaddr;
    int refcount;

    cow_t *next;
    cow_t *prev;
};


static cow_t cow_hash_table[HASH_TABLE_LEGTH] = { {0} };
static cow_t cow_nodes[FREE_NODES_LEGTH] = { {0} };

static cow_t* free_cow_nodes = 0;

void cow_init(void) {
    CREATE_FREE_OBJS_LIST(free_cow_nodes, cow_nodes, FREE_NODES_LEGTH);
}


uint32_t cow_void_cow_pte(uint32_t pte) {

    return NULL;
}

uint32_t cow_make_cow_pte(uint32_t pte) {
    if (PTE_IS_COW_PAGE(pte)) {
        // TODO: Search cow node and increment refcount
        
    } else {
        // TODO: Create cow node
    
    }
    return NULL;
}

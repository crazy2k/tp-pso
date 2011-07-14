#include <cow.h>
#include <mm.h>
#include <utils.h>


#define HASH_TABLE_LEGTH (PAGE_SIZE/sizeof(cow_t*))
#define FREE_NODES_LIST_LEGTH (4*PAGE_SIZE/sizeof(cow_t))


typedef struct cow_t cow_t;
struct cow_t {
    uint32_t phaddr;
    uint32_t refcount;

    cow_t *next;
    cow_t *prev;
};


struct {
    cow_t **table;
    uint32_t table_size;
    uint32_t objs_count;

    uint32_t (*hash_function)(uint32_t key);
    cow_t *(*create_node_function)(void);
    void (*free_node_function)(cow_t *node);
} cow_hash = { 0 };



uint32_t cow_hash_func(uint32_t key);

static cow_t *cow_hash_table[HASH_TABLE_LEGTH] = { NULL };
static cow_t cow_nodes[FREE_NODES_LIST_LEGTH] = { {0} };

static cow_t* free_cow_nodes = NULL;

static cow_t* fetch_node(uint32_t key);
static cow_t* insert_node(uint32_t key);
static void delete_node(uint32_t key);

cow_t *create_cow_node();
void free_cow_node(cow_t *node);

void cow_init(void) {
    CREATE_FREE_OBJS_LIST(free_cow_nodes, cow_nodes, FREE_NODES_LIST_LEGTH);

    cow_hash.table = cow_hash_table;
    cow_hash.table_size = HASH_TABLE_LEGTH;
    cow_hash.objs_count = 0;
    cow_hash.hash_function = &cow_hash_func;
    cow_hash.create_node_function = &create_cow_node;
    cow_hash.free_node_function = &free_cow_node;
}


uint32_t cow_delete_page_ref(uint32_t pte) {
    uint32_t key = PTE_PAGE_BASE(pte);
    cow_t* node = fetch_node(key);
    node->refcount--;

    if (node->refcount == 0) {
        delete_node(key);
        return 0;
    } else
        return node->refcount;
}

uint32_t cow_create_page_ref(uint32_t pte) {
    cow_t* node = insert_node(PTE_PAGE_BASE(pte));
    node->refcount = 2;

    return node->refcount;
}

uint32_t cow_add_page_ref(uint32_t pte) {
    cow_t* node = fetch_node(PTE_PAGE_BASE(pte));;

    return ++(node->refcount);
}


static cow_t* fetch_node(uint32_t key) {
   return NULL; 
}

static cow_t* insert_node(uint32_t key) {
   return NULL; 
}

static void delete_node(uint32_t key) {

}


uint32_t cow_hash_func(uint32_t key) {
    return key;
}

cow_t *create_cow_node(void) {
    return POP(&free_cow_nodes);
}

void free_cow_node(cow_t *node) {
    APPEND(&free_cow_nodes, node);
}


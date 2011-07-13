#include <cow.h>


typedef struct cow_t cow_t;
struct cow_t {
    uint32_t phaddr;
    int refcount;

    cow_t *next;
    cow_t *prev;
};

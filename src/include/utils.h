#ifndef __UTILS_H__

#define __UTILS_H__

#include "tipos.h"

typedef struct {
    void *buf;
    uint32_t offset;
    uint32_t remaining;
} circular_buf_t;


void *memcpy(void *dest, const void *src, size_t n);

void *memset(void * ptr, int value, size_t num);

int strcmp(char * src, char * dst);
int strncmp(const char *s1, const char *s2, size_t n);
char *strstr(const char *in, const char *str);
int strlen(const char* str);
int atoi(const char *str);

void custom_kpanic_msg(char* custom_msg);

uint32_t disable_interrupts();

void restore_eflags(uint32_t eflags);

char sc2ascii(unsigned char sc);

int write_to_circ_buff(circular_buf_t *cbuf, char* src, uint32_t size, uint32_t buf_size);
int read_from_circ_buff(char* dst, circular_buf_t *cbuf, uint32_t size, uint32_t buf_size);
void put_char_to_circ_buff(circular_buf_t *cbuf, char src, uint32_t buf_size);


#define IS_AT_LIST(node) ((node)->prev && (node)->next)

#define LINK_NODES(fst, sec) \
    (fst)->next = (sec); \
    (sec)->prev = (fst);

// UNLINK_NODE(<node type> **list, <node type> *node);
#define UNLINK_NODE(list, node) \
    if (*(list) != NULL) { \
        if ((node)->next == (node)) { \
            *(list) = NULL; \
        } \
        else { \
            if (*(list) == (node)) \
                *(list) = (node)->next; \
            (node)->next->prev = (node)->prev; \
            (node)->prev->next = (node)->next; \
        } \
        (node)->next = NULL; \
        (node)->prev = NULL; \
    }

// APPEND(<node type> **list, <node type> *node);
#define APPEND(list, node) \
    if (*(list) == NULL) { \
        (*(list)) = (node); \
        (node)->next = (node); \
        (node)->prev = (node); \
    } \
    else { \
        LINK_NODES((*(list))->prev, (node)); \
        LINK_NODES((node), (*(list))); \
    }

// POP(<node type> **list);
#define POP(list) \
    ({ \
        typeof(*(list)) _node = *(list); \
        UNLINK_NODE((list), _node); \
        _node; \
    })

#define NODES_ARRAY_TO_LIST(list, array, array_size) \
    ({ \
        (list) = NULL; \
        int i; \
        for (i = 0; i < array_size; i++) \
            APPEND(&list, &array[i]); \
    })

#define CREATE_FREE_OBJS_LIST(free_objs, objs, max_objs) \
    ({ \
        memset((objs), 0, sizeof((objs))); \
        NODES_ARRAY_TO_LIST(free_objs, objs, max_objs); \
    })



#endif

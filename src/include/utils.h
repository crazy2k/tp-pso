#ifndef __UTILS_H__

#define __UTILS_H__

#include "tipos.h"

void *memcpy(void *dest, const void *src, size_t n);

void *memset(void * ptr, int value, size_t num);

int strcmp(char * src, char * dst);

void custom_kpanic_msg(char* custom_msg);

uint32_t disable_interrupts();

void restore_eflags(uint32_t eflags);

#define LINK_NODES(fst, sec) \
    (fst)->next = (sec); \
    (sec)->prev = (fst);

// UNLINK_NODE(<node type> **list, <node type> *node);
#define UNLINK_NODE(list, node) \
    if (*(list) != NULL) { \
        if ((node)->next == (node)) { \
            (node)->next = NULL; \
            (node)->prev = NULL; \
            *(list) = NULL; \
        } \
        else { \
            if (*(list) == (node)) \
                *(list) = (node)->next; \
            (node)->next->prev = (node)->prev; \
            (node)->prev->next = (node)->next; \
        } \
    }

// APPEND(<node type> **list, <node type> *node);
#define APPEND(list, node) \
    if (*(list) == NULL) { \
        (*(list)) = (node); \
        (node)->next = (node); \
        (node)->prev = (node); \
    } \
    else { \
        LINK_NODES((*(list))->next, (node)); \
        LINK_NODES((node), (*(list))); \
    }

// POP(<node type> **list, <node type> **node);
#define POP(list, node) \
    *(node) = *(list); \
    UNLINK_NODE((list), *(node));


#endif

#ifndef __UTILS_H__

#define __UTILS_H__

#include <utils_common.h>
#include <device.h>

typedef struct {
    void *buf;
    uint32_t offset;
    uint32_t remaining;
} circular_buf_t;

void custom_kpanic_msg(char* custom_msg);

uint32_t disable_interrupts();

void restore_eflags(uint32_t eflags);

int write_to_circ_buff(circular_buf_t *cbuf, char* src, uint32_t size, uint32_t buf_size);
int read_from_circ_buff(char* dst, circular_buf_t *cbuf, uint32_t size, uint32_t buf_size);
void put_char_to_circ_buff(circular_buf_t *cbuf, char src, uint32_t buf_size);


// IS_AT_LIST(<node type> *list, <node type> *node);
#define IS_AT_LIST(list, node) ({ \
    typeof (list) __temp_list = (list); \
    bool __result = FALSE; \
    while (__temp_list) { \
        if (__temp_list == (node)) { \
            __result = TRUE; \
            break; \
        } \
        __temp_list = __temp_list->next; \
    } \
    __result; \
    })

// IS_EMPTY(<node type> *list);
#define IS_EMPTY(list) ((list) == NULL)

// UNLINK_NODE(<node type> *fst, <node type> *fst);
#define LINK_NODES(fst, sec) \
    (fst)->next = (sec); \
    (sec)->prev = (fst);

// UNLINK_NODE(<node type> **list_ptr, <node type> *node);
#define UNLINK_NODE(list_ptr, node) \
    if (*(list_ptr) != NULL) { \
        if ((node)->next == (node)) { \
            *(list_ptr) = NULL; \
        } \
        else { \
            if (*(list_ptr) == (node)) \
                *(list_ptr) = (node)->next; \
            (node)->next->prev = (node)->prev; \
            (node)->prev->next = (node)->next; \
        } \
        (node)->next = NULL; \
        (node)->prev = NULL; \
    }

// APPEND(<node type> **list_ptr, <node type> *node);
#define APPEND(list_ptr, node) \
    if (*(list_ptr) == NULL) { \
        (*(list_ptr)) = (node); \
        (node)->next = (node); \
        (node)->prev = (node); \
    } \
    else { \
        LINK_NODES((*(list_ptr))->prev, (node)); \
        LINK_NODES((node), (*(list_ptr))); \
    }

// POP(<node type> **list_ptr);
#define POP(list_ptr) \
    ({ \
        typeof(*(list_ptr)) _node = *(list_ptr); \
        UNLINK_NODE((list_ptr), _node); \
        _node; \
    })

#define SIZE(list_ptr) \
    ({ \
        typeof(list_ptr) _tip = (list_ptr); \
        typeof(list_ptr) _node = (_tip)->next; \
        int _size = 1; \
        while (_node != _tip) { \
            _size++; \
            _node = _node->next; \
        } \
        _size; \
    })

// NODES_ARRAY_TO_LIST(<node type> *list, <node type> array[], int size);
#define NODES_ARRAY_TO_LIST(list, array, size) \
    ({ \
        (list) = NULL; \
        int __i; \
        for (__i = 0; __i < (size); __i++) \
            APPEND(&(list), &(array)[__i]); \
    })

// CREATE_FREE_OBJS_LIST(<node type> *list, <node type> array[], int size);
#define CREATE_FREE_OBJS_LIST(list, array, size) \
    ({ \
        memset((array), 0, sizeof((array))); \
        NODES_ARRAY_TO_LIST((list), (array), (size)); \
    })



#endif

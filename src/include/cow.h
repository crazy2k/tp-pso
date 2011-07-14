#ifndef __COW_H__
#define __COW_H__

#include <tipos.h>


void cow_init(void);

uint32_t cow_delete_page_ref(uint32_t pte);
uint32_t cow_add_page_ref(uint32_t pte);
uint32_t cow_create_page_ref(uint32_t pte);

#endif

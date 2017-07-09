#ifndef __SWAP_H__
#define __SWAP_H__

#include <tipos.h>

void swap_init(void);
uint32_t swap_unload(void *vaddr);
void swap_load(uint32_t id, void *vaddr);

#endif

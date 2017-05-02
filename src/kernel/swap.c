#include <swap.h>
#include <errors.h>
#include <debug.h>
#include <utils.h>
#include <i386.h>

static uint32_t free_swapping_blocks = 1;

void swap_init(void) {
    /* Checkear presencia de archivo swap */
    /* Obtener tamaño de archivo */
    /* ¿Solo hacer swap de paginas de usuario, las paginas de kernel son compartidas entre cada procesos */
}

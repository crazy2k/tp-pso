#include <sem.h>
#include <loader.h>
#include <i386.h>
#include <debug.h>

void sem_wait(sem_t* s) {
    debug_printf("* sem: current value: %d\n", s->vl);
    s->vl--;
    debug_printf("* sem: after value: %d\n", s->vl);
    debug_printf("* sem: before if\n");
    if (s->vl < 0) {
        debug_printf("* sem: blocking\n");
        loader_enqueue((int *)(&s->q));
    }
    debug_printf("* sem: after if\n");
}

void sem_signaln(sem_t* s) {
    if (s->vl < 0)
        loader_unqueue((int *)(&s->q));
    s->vl++;
}

void sem_broadcast(sem_t* s) {
    while (s->q != -1)
        loader_unqueue((int *)(&s->q));
    s->vl = 0;
}

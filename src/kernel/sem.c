#include <sem.h>
#include <loader.h>
#include <i386.h>

void sem_wait(sem_t* s) {
    sti();

    s->vl--;
    if (s->vl < 0)
        loader_enqueue((int *)(&s->q));

    cli();
}

void sem_signaln(sem_t* s) {
    sti();

    if (s->vl < 0)
        loader_unqueue((int *)(&s->q));
    s->vl++;

    cli();
}

void sem_broadcast(sem_t* s) {
    sti();

    while (s->q != -1)
        loader_unqueue((int *)(&s->q));
    s->vl = 0;

    cli();
}

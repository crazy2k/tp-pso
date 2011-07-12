#include <sem.h>
#include <loader.h>
#include <i386.h>

void sem_wait(sem_t* s) {
    s->vl--;
    if (s->vl < 0)
        loader_enqueue((int *)(&s->q));
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

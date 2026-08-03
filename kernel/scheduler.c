// PingPongOS - PingPong Operating System

// Escalonador de tarefas prontas.

#include "lib/queue.h"

void sched_init() {}

struct task_t* scheduler(struct queue_t* ready_queue) {
    if (ready_queue == NULL) return NULL;

    struct task_t* next = queue_head(ready_queue);

    return next;
}

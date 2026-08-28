// PingPongOS - PingPong Operating System

// Escalonador de tarefas prontas.

#include "kernel/macros.h"
#include "kernel/tcb.h"
#include "lib/queue.h"

extern struct task_t* current_task;

const int AGING = -1;
const int MAX = -20;
const int MIN = 20;

void sched_init() {}

void sched_term() {}

void sched_set_dn_prio(struct task_t* task, int prio) {
    if ((prio < MAX) || (prio > MIN)) return;

    if (task == NULL)
        current_task->dn_prio = prio;
    else
        task->dn_prio = prio;
}

int sched_get_dn_prio(struct task_t* task) {
    if (task == NULL) return current_task->dn_prio;

    return task->dn_prio;
}

void sched_setprio(struct task_t* task, int prio) {
    if ((prio < MAX) || (prio > MIN)) return;

    if (task == NULL) {
        current_task->st_prio = prio;
        current_task->dn_prio = prio;
    } else {
        task->st_prio = prio;
        task->dn_prio = prio;
    }
}

int sched_getprio(struct task_t* task) {
    if (task == NULL) return current_task->st_prio;

    return task->st_prio;
}

struct task_t* scheduler(struct queue_t* ready_queue) {
    if (queue_size(ready_queue) == 0) return NULL;

    int next_prio, current_prio;
    struct task_t *next, *it;

    next = queue_head(ready_queue);
    next_prio = sched_get_dn_prio(next);

    it = queue_head(ready_queue);
    current_prio = sched_get_dn_prio(it);

    while (it != NULL) {
        if (current_prio < next_prio) {
            next = it;
            next_prio = current_prio;
        }

        sched_set_dn_prio(it, current_prio + AGING);
        it = queue_next(ready_queue);
        current_prio = sched_get_dn_prio(it);
    }

    ppos_debug("task %d (%s) with prio %d is the next task\n", next->id,
               next->name, next_prio);

    sched_set_dn_prio(next, sched_getprio(next));

    return next;
}

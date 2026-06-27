// PingPongOS - PingPong Operating System

#include "kernel/tcb.h"
#include "lib/queue.h"
#include "macros.h"

extern struct task_t* current_task;

const int AGING = -1;
const int MAX = -20;
const int MIN = 20;

void sched_init() {}

// muda a prioridade de uma tarefa
void sched_setprio(struct task_t* task, int prio) {
    if (prio > MAX && prio < MIN) {
        struct task_t* correct_task = (task) ? (task) : (current_task);

        ppos_debug("task %d (%s) set prio %d to task %d (%s)\n",
                   current_task->id, current_task->name, prio, correct_task->id,
                   correct_task->name);

        correct_task->st_prio = prio;
        correct_task->dn_prio = prio;
    }
}

// obtem a prioridade de uma tarefa
int sched_getprio(struct task_t* task) {
    if (task != NULL) return task->st_prio;

    return current_task->st_prio;
}

void sched_set_dynprio(struct task_t* task, int prio) {
    if (prio > MAX && prio < MIN) {
        if (task != NULL)
            task->dn_prio = prio;
        else
            current_task->dn_prio = prio;
    }
}

int sched_get_dynprio(struct task_t* task) {
    if (task != NULL) return task->dn_prio;

    return current_task->dn_prio;
}

// função escalonador: devolve a próxima tarefa a escalonar na fila
struct task_t* scheduler(struct queue_t* ready_queue) {
    int next_prio, current_prio;
    struct task_t *next, *it;

    next = queue_head(ready_queue);
    next_prio = sched_get_dynprio(next);

    it = queue_head(ready_queue);
    current_prio = sched_get_dynprio(it);

    while (it != NULL) {
        if (current_prio < next_prio) {
            next = it;
            next_prio = current_prio;
        }

        sched_set_dynprio(it, current_prio + AGING);
        it = queue_next(ready_queue);
        current_prio = sched_get_dynprio(it);
    }

    if (next == NULL) return NULL;

    ppos_debug("task %d (%s) with prio %d is the next task\n", next->id,
               next->name, next_prio);

    sched_set_dynprio(next, sched_getprio(next));

    return next;
}

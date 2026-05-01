// PingPongOS - PingPong Operating System

#include "lib/queue.h"

// Escalonador de tarefas prontas.

void sched_init() {}

// função escalonador: devolve a próxima tarefa a escalonar na fila
struct task_t* scheduler(struct queue_t* ready_queue) {
    return queue_head(ready_queue);
}

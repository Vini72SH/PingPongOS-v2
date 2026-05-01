// PingPongOS - PingPong Operating System

#include "kernel/macros.h"
#include "kernel/scheduler.h"
#include "kernel/task.h"
#include "kernel/tcb.h"
#include "lib/queue.h"

// Dispatcher: gerencia os estados das tarefas.

extern void(user_main)(void* args);
extern struct task_t kernel_task;
extern struct task_t* current_task;

struct queue_t* ready;
struct queue_t* suspended;

// executa a tarefa indicada: a retira da fila de prontas,
// muda seu status para RODANDO e transfere a CPU para ela.
void task_run(struct task_t* task) {
    queue_del(ready, task);
    task->status = RUNNING;
    task_switch(task);
}

// a tarefa atual libera a CPU para o dispatcher,
// voltando para a fila de prontas
void task_yield() {
    current_task->status = READY;
    queue_add(ready, current_task);
    task_switch(&kernel_task);
}

// suspende a tarefa atual: a retira da fila de prontas,
// a insere na fila "queue" (se não for NULL) e retorna
// ao dispatcher.
void task_suspend(struct queue_t* queue) {
    current_task->status = SUSPENDED;

    if (queue != NULL) queue_add(queue, current_task);

    task_switch(&kernel_task);
}

// acorda uma tarefa: a retira da fila onde se encontra
// suspensa (se estiver em uma) e a insere na fila de
// prontas, para retomar (ou iniciar) sua execução.
void task_awake(struct task_t* task) {
    if (queue_has(suspended, task)) queue_del(suspended, task);

    task->status = READY;
    queue_add(ready, task);
}

// encerra a execução da tarefa atual, informando um
// "exit code", e retorna ao dispatcher.
void task_exit(int exit_code) {
    current_task->status = FINISHED;
    current_task->exit_code = exit_code;

    task_switch(&kernel_task);
}

void dispatcher_init() {
    ppos_debug("subsystem dispatcher initiated\n");

    ready = queue_create();
    if (ready == NULL) return;

    suspended = queue_create();
    if (suspended == NULL) return;
}

void dispatcher() {
    ppos_debug("dispatcher started\n");

    struct task_t* next_task;

    task_create("user", user_main, NULL);
    while (queue_size(ready) > 0) {
        next_task = scheduler(ready);

        if (next_task != NULL) {
            task_run(next_task);

            switch (next_task->status) {
                case READY:
                    break;

                case SUSPENDED:
                    break;

                case FINISHED:
                    task_destroy(next_task);
                    break;

                default:
                    break;
            }
        }
    }

    ppos_debug("dispatcher stopping, no more user tasks\n");

    queue_destroy(suspended);
    queue_destroy(ready);
}

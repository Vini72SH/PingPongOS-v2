// PingPongOS - PingPong Operating System

#include "kernel/ctx.h"
#include "kernel/macros.h"
#include "kernel/scheduler.h"
#include "kernel/task.h"
#include "kernel/tcb.h"
#include "lib/queue.h"
#include "time.h"

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
    current_task->quantum = QUANTUM;
    queue_add(ready, current_task);
    task_switch(&kernel_task);
}

// suspende a tarefa atual: a retira da fila de prontas,
// a insere na fila "queue" (se não for NULL) e retorna
// ao dispatcher.
void task_suspend(struct queue_t* queue) {
    if (current_task != NULL) {
        current_task->status = SUSPENDED;

        if (queue != NULL) queue_add(queue, current_task);

        task_switch(&kernel_task);
    }
}

// acorda uma tarefa: a retira da fila onde se encontra
// suspensa (se estiver em uma) e a insere na fila de
// prontas, para retomar (ou iniciar) sua execução.
void task_awake(struct task_t* task) {
    if (task == NULL) return;

    if (queue_has(suspended, task)) queue_del(suspended, task);

    task->status = READY;
    queue_add(ready, task);
}

// encerra a execução da tarefa atual, informando um
// "exit code", e retorna ao dispatcher.
void task_exit(int exit_code) {
    if (current_task != NULL) {
        current_task->lifetime = systime() - current_task->lifetime;
        current_task->status = FINISHED;
        current_task->exit_code = exit_code;

        struct task_t* aux;
        aux = queue_head(current_task->waiting);
        while (aux != NULL) {
            queue_del(current_task->waiting, aux);
            task_awake(aux);
            aux = queue_item(current_task->waiting);
        }

        task_switch(&kernel_task);
    }
}

int task_wait(struct task_t* task) {
    if (task == NULL) return ERROR;
    if (task->status == FINISHED) return task->exit_code;

    task_suspend(task->waiting);

    return task->exit_code;
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

    struct task_t *main, *next_task;

    main = task_create("user", user_main, NULL);
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
                    printk(
                        "PPOS: task %d (%s) exit code %d, %d ms elapsed time, "
                        "%d ms cpu time, %d activations\n",
                        next_task->id, next_task->name, next_task->exit_code,
                        next_task->lifetime, next_task->cputime,
                        next_task->activations);
                    break;

                default:
                    break;
            }
        }
    }

    task_destroy(main);

    kernel_task.lifetime = systime() - kernel_task.lifetime;
    printk(
        "PPOS: task %d (%s) exit code %d, %d ms elapsed time, %d ms cpu time, "
        "%d activations\n",
        kernel_task.id, kernel_task.name, kernel_task.exit_code,
        kernel_task.lifetime, kernel_task.cputime, kernel_task.activations);

    queue_destroy(kernel_task.waiting);

    ppos_debug("dispatcher stopping, no more user tasks\n");

    queue_destroy(suspended);
    queue_destroy(ready);
}

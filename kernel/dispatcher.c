// PingPongOS - PingPong Operating System

// Dispatcher: gerencia os estados das tarefas.

#include "kernel/dispatcher.h"

#include "kernel/macros.h"
#include "kernel/scheduler.h"
#include "kernel/task.h"
#include "kernel/tcb.h"
#include "kernel/time.h"
#include "lib/queue.h"

extern void(user_main)(void* args);

extern struct task_t kernel_task;
extern struct task_t* current_task;

struct queue_t* ready_tasks;
struct queue_t* suspended_tasks;
struct queue_t* sleeping_tasks;

// inicia o subsistema dispatcher
// (chamada pelo núcleo na inicialização).
void dispatcher_init() {
    ready_tasks = queue_create();
    if (ready_tasks == NULL) return;

    suspended_tasks = queue_create();
    if (suspended_tasks == NULL) return;

    sleeping_tasks = queue_create();
    if (sleeping_tasks == NULL) return;

    ppos_debug("subsystem dispatcher initiated\n");
}

// encerra o subsistema dispatcher
// (chamada pelo núcleo no encerramento).
void dispatcher_term() {
    queue_destroy(sleeping_tasks);
    queue_destroy(suspended_tasks);
    queue_destroy(ready_tasks);
}

// transfere a CPU da tarefa atual para outra tarefa; se task_id == 0,
// transfere para o núcleo. Ignora sem erro se "task" já tiver terminado.
// Retorno: NOERROR (0) ou ERROR (<0)
int task_switch(struct task_t* task) {
    struct task_t* running_task = current_task;
    struct task_t* next_task;

    if (task != NULL)
        next_task = task;
    else
        next_task = current_task->creator;

    if (next_task->status == FINISHED) return NOERROR;

    ppos_debug("task %d (%s) switch to task %d (%s)\n", running_task->id,
               running_task->name, next_task->id, next_task->name);

    current_task = next_task;
    next_task->activations++;

    int status = ctx_switch(&running_task->context, &next_task->context);
    return status;
}

// executa a tarefa indicada: retira-a da fila de prontas, muda seu status
// para RODANDO e transfere a CPU para ela.
void task_run(struct task_t* task) {
    if (task == NULL) return;

    queue_del(ready_tasks, task);
    task->current_queue = NULL;
    task->quantum = 10;
    task->status = RUNNING;
    task_switch(task);
}

// suspende a tarefa atual: retira-a da fila de prontas, muda seu status para
// SUSPENSA, a insere na fila "queue" (se não for NULL) e retorna ao dispatcher.
void task_suspend(struct queue_t* queue) {
    current_task->status = SUSPENDED;
    current_task->current_queue = queue;
    if (queue != NULL) queue_add(queue, current_task);
    task_switch(&kernel_task);
}

// acorda uma tarefa: retira-a da fila onde se encontra suspensa (se estiver
// em uma fila), muda seu status para PRONTA e a insere na fila de prontas,
// para retomar (ou iniciar) sua execução.
void task_awake(struct task_t* task) {
    queue_del(task->current_queue, task);
    task->status = READY;
    queue_add(ready_tasks, task);
}

void waking_up_tasks() {
    struct task_t* aux;

    long int current_time = time();
    aux = queue_head(sleeping_tasks);
    while (aux != NULL) {
        if (aux->waking_up_in <= current_time) {
            task_awake(aux);
            aux = queue_item(sleeping_tasks);
        } else {
            aux = queue_next(sleeping_tasks);
        }
    }
}

// executa o dispatcher (chamada pelo núcleo após a inicialização).
void dispatcher() {
    ppos_debug("dispatcher started\n");

    struct task_t *main, *next;

    main = task_create("user", user_main, NULL);
    while ((queue_size(ready_tasks) > 0) || (queue_size(suspended_tasks) > 0) ||
           (queue_size(sleeping_tasks) > 0)) {
        next = scheduler(ready_tasks);
        if (next != NULL) {
            task_run(next);

            if (next->status == FINISHED) {
                printk(
                    "PPOS: task %d (%s), %d ms run, %d ms cpu, %d acts, exit "
                    "code "
                    "%d\n",
                    next->id, next->name, next->lifetime, next->cputime,
                    next->activations, next->exit_code);
            }
        }

        waking_up_tasks();
    }

    task_destroy(main);

    ppos_debug("dispatcher stopping, no more user tasks\n");

    printk(
        "PPOS: task %d (%s), %d ms run, %d ms cpu, %d acts, exit "
        "code "
        "%d\n",
        kernel_task.id, kernel_task.name, kernel_task.lifetime,
        kernel_task.cputime, kernel_task.activations, kernel_task.exit_code);
}

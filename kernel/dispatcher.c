// PingPongOS - PingPong Operating System

// Dispatcher: gerencia os estados das tarefas.

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
struct queue_t* finished_tasks;

void dispatcher_init() {
    ready_tasks = queue_create();
    if (ready_tasks == NULL) return;

    suspended_tasks = queue_create();
    if (suspended_tasks == NULL) return;

    finished_tasks = queue_create();
    if (finished_tasks == NULL) return;

    ppos_debug("subsystem dispatcher initiated\n");
}

void dispatcher_term() {
    struct task_t* aux;

    aux = queue_head(finished_tasks);
    while (aux != NULL) {
        task_destroy(aux);
        aux = queue_next(finished_tasks);
    }

    queue_destroy(finished_tasks);
    queue_destroy(suspended_tasks);
    queue_destroy(ready_tasks);
}

void task_run(struct task_t* task) {
    if (task == NULL) return;

    queue_del(ready_tasks, task);
    task->status = RUNNING;
    task_switch(task);
}

void task_yield() {
    current_task->status = READY;
    queue_add(ready_tasks, current_task);
    task_switch(&kernel_task);
}

void task_suspend(struct queue_t* queue) {
    current_task->status = SUSPENDED;
    if (queue != NULL) queue_add(queue, current_task);
    task_switch(&kernel_task);
}

void task_awake(struct task_t* task) {
    queue_del(suspended_tasks, task);
    task->status = READY;
    queue_add(ready_tasks, task);
}

void task_exit(int exit_code) {
    current_task->status = FINISHED;
    current_task->lifetime = time() - current_task->lifetime;
    current_task->exit_code = exit_code;
    task_switch(&kernel_task);
}

void dispatcher() {
    struct task_t* next;

    task_create("user", user_main, NULL);
    while ((queue_size(ready_tasks) > 0) || (queue_size(suspended_tasks) > 0)) {
        next = scheduler(ready_tasks);
        if (next != NULL) {
            task_run(next);

            if (next->status == FINISHED) {
                queue_add(finished_tasks, next);
                printk(
                    "PPOS: task %d (%s), %d ms run, %d ms cpu, %d acts, exit "
                    "code "
                    "%d\n",
                    next->id, next->name, next->lifetime, next->cputime,
                    next->activations, next->exit_code);
            }
        }
    }

    ppos_debug("dispatcher stopping, no more user tasks\n");

    printk(
        "PPOS: task %d (%s), %d ms run, %d ms cpu, %d acts, exit "
        "code "
        "%d\n",
        kernel_task.id, kernel_task.name, kernel_task.lifetime,
        kernel_task.cputime, kernel_task.activations, kernel_task.exit_code);
}

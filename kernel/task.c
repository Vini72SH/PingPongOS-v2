// PingPongOS - PingPong Operating System

// Gerência básica de tarefas.
#include <valgrind/valgrind.h>

#include "kernel/macros.h"
#include "kernel/memory.h"
#include "kernel/tcb.h"
#include "kernel/time.h"
#include "lib/queue.h"

const int STACKSIZE = 16834;

long int uid = 1;

struct task_t kernel_task = {0};
struct task_t* current_task = NULL;

extern struct queue_t* ready_tasks;

void task_init() {
    kernel_task.id = 0;
    kernel_task.name = "kernel";
    kernel_task.status = RUNNING;
    kernel_task.creator = NULL;
    kernel_task.vg_id = 0;
    kernel_task.st_prio = MAX;
    kernel_task.dn_prio = MAX;
    kernel_task.quantum = 0;
    kernel_task.type = KERNEL;
    kernel_task.lifetime = time();
    kernel_task.cputime = 0;
    kernel_task.activations = 1;
    kernel_task.exit_code = 0;
    current_task = &kernel_task;

    ppos_debug("subsystem task initiated\n");
}

void task_term() {}
struct task_t* task_create(char* name, void (*entry)(void*), void* arg) {
    struct task_t* new_task;

    new_task = mem_alloc(sizeof(struct task_t));
    if (new_task == NULL) return NULL;

    char* stack = mem_alloc(STACKSIZE);
    if (stack == NULL) {
        mem_free(new_task);
        return NULL;
    }

    new_task->id = uid++;
    new_task->name = name;
    new_task->stack = stack;

    new_task->vg_id =
        VALGRIND_STACK_REGISTER(new_task->stack, new_task->stack + STACKSIZE);

    if (ctx_create(&new_task->context, entry, arg, new_task->stack,
                   STACKSIZE) != NOERROR) {
        mem_free(new_task->stack);
        mem_free(new_task);
        return NULL;
    }

    new_task->creator = current_task;
    new_task->status = READY;
    new_task->st_prio = 0;
    new_task->dn_prio = 0;
    new_task->quantum = 10;
    new_task->type = USER;
    new_task->lifetime = time();
    new_task->cputime = 0;
    new_task->activations = 0;
    new_task->exit_code = 0;

    ppos_debug("task %d (%s) create task %d (%s)\n", current_task->id,
               current_task->name, new_task->id, new_task->name);

    queue_add(ready_tasks, new_task);

    return new_task;
}

int task_destroy(struct task_t* task) {
    if (task == NULL) return ERROR;
    VALGRIND_STACK_DEREGISTER(task->vg_id);
    mem_free(task->stack);
    mem_free(task);
    return NOERROR;
}

int task_id(struct task_t* task) {
    if (task == NULL) return current_task->id;
    return task->id;
}

char* task_name(struct task_t* task) {
    if (task == NULL) return current_task->name;
    return task->name;
}
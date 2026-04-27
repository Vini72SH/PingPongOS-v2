// PingPongOS - PingPong Operating System

#include "kernel/macros.h"
#include "kernel/task.h"
#include "kernel/tcb.h"

// Dispatcher: gerencia os estados das tarefas.

extern void(user_main)(void* args);

void dispatcher_init() { ppos_debug("subsystem dispatcher initiated\n"); }

void dispatcher() {
    ppos_debug("dispatcher started\n");

    struct task_t* user_task;
    user_task = task_create("user", user_main, NULL);

    task_switch(user_task);

    user_task->status = FINISHED;
    task_destroy(user_task);
}

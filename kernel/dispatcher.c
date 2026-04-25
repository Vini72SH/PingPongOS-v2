// PingPongOS - PingPong Operating System

#include "kernel/macros.h"
#include "kernel/task.h"
#include "kernel/tcb.h"
#include "test/pingpong-task1.c"

// Dispatcher: gerencia os estados das tarefas.

void dispatcher_init() {
#ifdef DEBUG
    ppos_debug("subsystem dispatcher initiated\n");
#endif
}

void dispatcher() {
#ifdef DEBUG
    ppos_debug("dispatcher started\n");
#endif

    struct task_t* user_task;
    user_task = task_create("user", user_main, NULL);

    task_switch(user_task);

    user_task->status = FINISHED;
    task_destroy(user_task);
}

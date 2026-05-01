// PingPongOS - PingPong Operating System

#include <valgrind/valgrind.h>

#include "ctx.h"
#include "kernel/macros.h"
#include "lib/libc.h"
#include "lib/queue.h"
#include "memory.h"
#include "tcb.h"

#define STACKSIZE 4096

extern void (*user_main)(void* args);

long int uid = 1;
struct task_t kernel_task = {0};
struct task_t* current_task = NULL;

extern struct queue_t* ready;

/*
 * Esta função é chamada por ppos.c:ppos_init e inicia as variáveis necessárias
 * à gestão de tarefas. Neste momento, sua principal responsabilidade é definir
 * uma tarefa para o fluxo de execução do núcleo, que pode ser chamada de
 * task_kernel, com nome “kernel” e ID 0. Essa tarefa pode ser vista como o
 * equivalente da função main no programa contexts.c do projeto anterior. A
 * tarefa do núcleo não deve ser criada com task_create, pois não usa uma pilha
 * separada nem uma função separada.
 */
void task_init() {
    kernel_task.id = 0;
    kernel_task.name = "kernel";
    kernel_task.status = RUNNING;

    current_task = &kernel_task;

    ppos_debug("subsystem task initiated\n");
}

// cria uma nova tarefa: "name" é o nome da tarefa, "entry"
// é a função que ela irá executar e "arg" aponta para o valor
// recebido por "entry" ao iniciar (pode ser NULL).
// retorno: ptr para o descritor da tarefa ou NULL se houver erro
struct task_t* task_create(char* name, void (*entry)(void*), void* arg) {
    struct task_t* new_task;

    new_task = mem_alloc(sizeof(struct task_t));
    if (new_task == NULL) return NULL;

    char* stack = mem_alloc(STACKSIZE);
    if (stack == NULL) return NULL;

    new_task->id = uid++;
    new_task->name = name;
    new_task->stack = stack;
    ctx_create(&new_task->context, entry, arg, new_task->stack, STACKSIZE);
    new_task->creator = current_task;
    new_task->status = READY;
    new_task->exit_code = 0;

    new_task->vg_id =
        VALGRIND_STACK_REGISTER(new_task->stack, new_task->stack + STACKSIZE);

    queue_add(ready, new_task);

    ppos_debug("task %d (%s) create task %d (%s)\n", current_task->id,
               current_task->name, new_task->id, new_task->name);

    return new_task;
}

// destroi uma tarefa e libera seus recursos;
// somente deve atuar sobre tarefas terminadas.
// Retorno: NOERROR (0) ou ERROR (<0)
int task_destroy(struct task_t* task) {
    if (task == NULL) return NOERROR;
    if (task->status != FINISHED) return ERROR;

    ppos_debug("task %d (%s) destroy task %d (%s)\n", current_task->id,
               current_task->name, task->id, task->name);

    VALGRIND_STACK_DEREGISTER(task->vg_id);

    mem_free(task->stack);
    mem_free(task);

    task = NULL;

    return NOERROR;
}

// transfere a cpu da tarefa atual para outra tarefa;
// se task == NULL, transfere para a tarefa que a criou.
// ignora sem erro se "task" já tiver terminado.
// Retorno: NOERROR (0) ou ERROR (<0)
int task_switch(struct task_t* task) {
    struct task_t* running_task = current_task;
    struct task_t* next_task;

    if (task != NULL)
        next_task = task;
    else
        next_task = current_task->creator;

    current_task = next_task;

    ppos_debug("task %d (%s) switch to %d (%s)\n", running_task->id,
               running_task->name, next_task->id, next_task->name);

    next_task->status = RUNNING;
    return ctx_swap(&running_task->context, &next_task->context);
}

// informa o ID de uma tarefa (ou da tarefa atual se NULL)
int task_id(struct task_t* task) {
    if (task != NULL) return task->id;

    return current_task->id;
}

// informa o nome de uma tarefa (ou da tarefa atual se NULL)
char* task_name(struct task_t* task) {
    if (task != NULL) return task->name;

    return current_task->name;
}
// PingPongOS - PingPong Operating System

// Gerência básica de tarefas.
#include <valgrind/valgrind.h>

#include "kernel/macros.h"
#include "kernel/memory.h"
#include "kernel/tcb.h"
#include "kernel/time.h"
#include "lib/queue.h"
#include "kernel/dispatcher.h"

const int STACKSIZE = 16384;

long int uid = 1;

struct task_t kernel_task = {0};
struct task_t* current_task = NULL;

extern struct queue_t* ready_tasks;

// inicializa o subsistema de tarefas.
// (chamada pelo núcleo na inicialização).
void task_init() {
    kernel_task.id = 0;
    kernel_task.name = "kernel";
    kernel_task.status = RUNNING;
    kernel_task.creator = NULL;
    kernel_task.vg_id = 0;
    kernel_task.st_prio = 0;
    kernel_task.dn_prio = 0;
    kernel_task.quantum = 0;
    kernel_task.lifetime = time();
    kernel_task.cputime = 0;
    kernel_task.activations = 1;
    kernel_task.exit_code = 0;
    kernel_task.type = KERNEL;
    current_task = &kernel_task;

    ppos_debug("subsystem task initiated\n");
}

// encerra o subsistema de tarefas.
// (chamada pelo núcleo no encerramento).
void task_term() {}

// cria uma nova tarefa: "name" é o nome da tarefa, "entry" é a função que
// ela irá executar e "arg" aponta para o valor recebido por "entry" ao
// iniciar (pode ser NULL).
// Retorno: ptr para a tarefa ou NULL se houver erro.
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
    new_task->quantum = QUANTUM;
    new_task->type = USER;
    new_task->lifetime = time();
    new_task->cputime = 0;
    new_task->activations = 0;
    new_task->exit_code = 0;

    if (queue_add(ready_tasks, new_task) != NOERROR) {
        VALGRIND_STACK_DEREGISTER(new_task->vg_id);
        mem_free(new_task->stack);
        mem_free(new_task);
        return NULL;
    }

    ppos_debug("task %d (%s) create task %d (%s)\n", current_task->id,
               current_task->name, new_task->id, new_task->name);

    return new_task;
}

// destrói uma tarefa e libera seus recursos; somente deve atuar sobre tarefas
// terminadas. Retorno: NOERROR (0) ou ERROR (<0).
int task_destroy(struct task_t* task) {
    if (task == NULL) return ERROR;
    if (task->status != FINISHED) return ERROR;

    VALGRIND_STACK_DEREGISTER(task->vg_id);
    mem_free(task->stack);
    mem_free(task);
    return NOERROR;
}

// informa o ID de uma tarefa (ou da tarefa atual se task == NULL)
int task_id(struct task_t* task) {
    if (task == NULL) return current_task->id;
    return task->id;
}

// informa o nome de uma tarefa (ou da tarefa atual se task == NULL)
char* task_name(struct task_t* task) {
    if (task == NULL) return current_task->name;
    return task->name;
}

// a tarefa atual libera a CPU e volta para a fila de prontas; a execução
// retorna ao núcleo/dispatcher.
void task_yield() {
    current_task->status = READY;
    current_task->quantum = QUANTUM;
    queue_add(ready_tasks, current_task);
    task_switch(&kernel_task);
}

// suspende a tarefa atual até que a tarefa task termine; a execução retorna
// ao núcleo/dispatcher. Se a tarefa task já terminou, retorna sem suspender.
// Retorno: exit code tarefa que terminou ou ERROR.
int task_wait(struct task_t* task) {}

// suspende a tarefa atual por t milissegundos; a execução retorna ao
// núcleo/dispatcher.
void task_sleep(int t) {}

// encerra a execução da tarefa atual, informando um código de encerramento
// (exit_code); a execução retorna ao núcleo/dispatcher.
void task_exit(int exit_code) {
    if (current_task != NULL) {
        current_task->status = FINISHED;
        current_task->lifetime = time() - current_task->lifetime;
        current_task->exit_code = exit_code;
        task_switch(&kernel_task);
    }
}

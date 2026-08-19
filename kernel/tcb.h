// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 2.0 -- Junho de 2025

// Descritor de tarefas (TCB - Task Control Block).

#ifndef __PPOS_TCB__
#define __PPOS_TCB__

#include "ctx.h"
#include "lib/queue.h"

enum task_type { KERNEL, USER };
enum task_status { NONE, READY, RUNNING, SLEEPING, SUSPENDED, FINISHED };

typedef enum task_type task_type;

// Task Control Block (TCB), infos sobre uma tarefa
struct task_t {
    int id;                  // identificador da tarefa
    char* name;              // nome da tarefa
    char* stack;             // stack da tarefa
    struct ctx_t context;    // contexto armazenado da tarefa
    struct task_t* creator;  // ponteiro para a tarefa pai
    int status;              // status da tarefa
    int vg_id;               // ID da pilha da tarefa no Valgrind
    int st_prio;             // prioridade estática da tarefa
    int dn_prio;             // prioridade dinâmica da tarefa
    int quantum;             // quantum atual da tarefa
    int lifetime;            // tempo de vida da tarefa
    int cputime;             // tempo de execução da tarefa
    int activations;         // qtd de ativações da tarefa
    int exit_code;           // código de saída da tarefa
    int waking_up_in;        // quando a tarefa deve acordar
    task_type type;          // tipo da tarefa

    struct queue_t* waiting;
};

#endif

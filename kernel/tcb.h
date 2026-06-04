// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 2.0 -- Junho de 2025

// Descritor de tarefas (TCB - Task Control Block).

#ifndef __PPOS_TCB__
#define __PPOS_TCB__

#include "ctx.h"

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
    int exit_code;           // exit code da tarefa
    int vg_id;               // registro da stack no valgrind
    int st_prio;             // prioridade estática da tarefa
    int dn_prio;             // prioridade dinâmica da tarefa
    int quantum;             // Ticks de execução
    long int lifetime;       // Tempo de vida da tarefa (milissegundos)
    long int cputime;        // Tempo de CPU gasto pela tarefa (milissegundos)
    long int activations;    // Ativações da tarefa

    task_type type;  // Tipo da tarefa
};

#endif

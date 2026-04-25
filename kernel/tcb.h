// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 2.0 -- Junho de 2025

// Descritor de tarefas (TCB - Task Control Block).

#ifndef __PPOS_TCB__
#define __PPOS_TCB__

#include "ctx.h"

enum task_status { NONE, READY, RUNNING, SLEEPING, SUSPENDED, FINISHED };

// Task Control Block (TCB), infos sobre uma tarefa
struct task_t {
    int id;                  // identificador da tarefa
    char* name;              // nome da tarefa
    char* stack;             // stack da tarefa
    struct ctx_t context;    // contexto armazenado da tarefa
    struct task_t* creator;  // ponteiro para a tarefa pai
    int status;              // status da tarefa
};

#endif

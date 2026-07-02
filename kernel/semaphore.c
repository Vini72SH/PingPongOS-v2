// PingPongOS - PingPong Operating System

#include "kernel/semaphore.h"

#include <stdio.h>

#include "kernel/ctx.h"
#include "kernel/dispatcher.h"
#include "kernel/macros.h"
#include "kernel/memory.h"
#include "kernel/tcb.h"
#include "lib/queue.h"

long int sid;

extern struct task_t* current_task;
struct queue_t* semaphores = NULL;

struct semaphore_t {
    int lock;
    int value;
    long int id;
    struct queue_t* waiting;
};

void spin_lock(int* lock) { while (__sync_fetch_and_or(lock, 1)); }

void spin_unlock(int* lock) { (*lock) = 0; }

void sem_init() {
    sid = 0;
    semaphores = queue_create();
    if (semaphores == NULL) return;
}

// Cria um novo semáforo, inicializado com value >= 0.
// Retorno: ptr para o semáforo ou NULL (erro).
struct semaphore_t* sem_create(int value) {
    struct semaphore_t* sem;

    if (semaphores == NULL) {
        sid = 0;
        semaphores = queue_create();
        if (semaphores == NULL) return NULL;
    }

    sem = mem_alloc(sizeof(struct semaphore_t));
    if (sem == NULL) return NULL;

    sem->id = sid++;
    sem->lock = 0;
    sem->value = value;
    sem->waiting = queue_create();

    if (sem->waiting == NULL) {
        mem_free(sem);
        return NULL;
    }

    queue_add(semaphores, sem);

    return sem;
}

// Requisita acesso a um semáforo
// Retorno: NOERROR (0) ou ERROR (<0)
int sem_down(struct semaphore_t* s) {
    if (s == NULL) return ERROR;
    if (!queue_has(semaphores, s)) return ERROR;

    int suspend = 0;

    spin_lock(&s->lock);
    s->value--;
    if (s->value < 0) {
        suspend = 1;
    }
    spin_unlock(&s->lock);

    if (suspend) {
        printf("A tarefa %d (%s) será suspensa\n", current_task->id,
               current_task->name);
        task_suspend(s->waiting);
    }

    return NOERROR;
}

// libera o acesso a um semáforo
// Retorno: NOERROR (0) ou ERROR (<0)
int sem_up(struct semaphore_t* s) {
    if (s == NULL) return ERROR;
    if (!queue_has(semaphores, s)) return ERROR;

    struct task_t* task = NULL;

    spin_lock(&s->lock);
    s->value++;
    if (s->value <= 0) {
        task = queue_head(s->waiting);
        if (task != NULL) {
            queue_del(s->waiting, task);
            task_awake(task);
        }
    }

    spin_unlock(&s->lock);

    if (task != NULL) {
        printf("A tarefa %d (%s) será acordada\n", task->id, task->name);
    }

    return NOERROR;
}

// destrói um semáforo, liberando recursos e tarefas bloqueadas
// Retorno: NOERROR (0) ou ERROR (<0)
int sem_destroy(struct semaphore_t* s) {
    if (s == NULL) return ERROR;
    if (!queue_has(semaphores, s)) return ERROR;

    struct task_t* aux;
    aux = queue_head(s->waiting);
    while (aux != NULL) {
        queue_del(s->waiting, aux);
        task_awake(aux);
        printf("A tarefa %d (%s) será acordada após a destruição do semáforo\n",
               aux->id, aux->name);
        aux = queue_head(s->waiting);
    }

    // Remove o semáforo da lista global de semáforos
    queue_del(semaphores, s);
    queue_destroy(s->waiting);
    mem_free(s);

    if (queue_size(semaphores) == 0) {
        queue_destroy(semaphores);
        semaphores = NULL;
    }

    return NOERROR;
}

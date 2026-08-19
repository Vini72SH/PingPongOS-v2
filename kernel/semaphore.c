// PingPongOS - PingPong Operating System

// Semáforos e spinlocks

#include "kernel/semaphore.h"

#include "kernel/dispatcher.h"
#include "kernel/macros.h"
#include "kernel/memory.h"
#include "kernel/tcb.h"
#include "lib/map.h"
#include "lib/queue.h"

const int NUM_SEMAPHORES = 32;

struct semaphore_t {
    int id;
    int lock;
    int value;
    struct queue_t* tasks;
};

struct map_t* semaphores = NULL;

void sem_init() { semaphores = map_create(NUM_SEMAPHORES); }

void sem_term() { map_destroy(semaphores); }

// trava um spin-lock (busy wait)
void spin_lock(int* lock) { while (__sync_fetch_and_or(lock, 1)); }

// libera um spin-lock
void spin_unlock(int* lock) { (*lock) = 0; }

// Cria um novo semáforo, inicializado com value >= 0.
// Retorno: descritor do semáforo ou -1 (erro).
int sem_create(int value) {
    struct semaphore_t* sem;

    sem = mem_alloc(sizeof(struct semaphore_t));
    if (sem == NULL) return ERROR;

    sem->id = map_put(semaphores, sem);
    sem->lock = 0;
    sem->value = value;
    sem->tasks = queue_create();

    return sem->id;
}

// destrói um semáforo, liberando recursos e tarefas bloqueadas
// Retorno: NOERROR (0) ou ERROR (<0)
int sem_destroy(int sem_id) {
    struct semaphore_t* sem = map_del(semaphores, sem_id);
    if (sem == NULL) return ERROR;

    struct task_t* aux;
    aux = queue_head(sem->tasks);
    while (aux != NULL) {
        task_awake(aux);
        queue_del(sem->tasks, aux);
        aux = queue_head(sem->tasks);
    }

    queue_destroy(sem->tasks);
    mem_free(sem);

    return NOERROR;
}

// Requisita acesso a um semáforo
// Retorno: NOERROR (0) ou ERROR (<0)
int sem_down(int sem_id) {
    struct semaphore_t* sem = map_get(semaphores, sem_id);
    if (sem == NULL) return ERROR;

    int suspend = 0;

    spin_lock(&sem->lock);
    sem->value--;
    if (sem->value < 0) suspend = 1;
    spin_unlock(&sem->lock);

    if (suspend) task_suspend(sem->tasks);

    return NOERROR;
}

// libera o acesso a um semáforo
// Retorno: NOERROR (0) ou ERROR (<0)
int sem_up(int sem_id) {
    struct semaphore_t* sem = map_get(semaphores, sem_id);
    if (sem == NULL) return ERROR;

    struct task_t* task = NULL;

    spin_lock(&sem->lock);
    sem->value++;
    if (sem->value <= 0) {
        task = queue_head(sem->tasks);
        queue_del(sem->tasks, task);
        task_awake(task);
    }
    spin_unlock(&sem->lock);

    return NOERROR;
}

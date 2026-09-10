// PingPongOS - PingPong Operating System

// Semáforos e spinlocks

#include "kernel/semaphore.h"

#include "kernel/dispatcher.h"
#include "kernel/macros.h"
#include "kernel/memory.h"
#include "kernel/tcb.h"
#include "lib/map.h"
#include "lib/queue.h"

extern struct task_t kernel_task;
extern struct task_t* current_task;

const int NUM_SEMAPHORES = 32;

struct semaphore_t {
    int id;
    int lock;
    int value;
    struct queue_t* tasks;
};

struct map_t* semaphores = NULL;

void sem_init() {
    semaphores = map_create(NUM_SEMAPHORES);
    ppos_debug("subsystem semaphores initiated\n");
}

void sem_term() { map_destroy(semaphores); }

// trava um spin-lock (busy wait)
void spin_lock(int* lock) { while (__sync_fetch_and_or(lock, 1)); }

// libera um spin-lock
void spin_unlock(int* lock) { (*lock) = 0; }

// Cria um novo semáforo, inicializado com value >= 0.
// Retorno: descritor do semáforo ou -1 (erro).
int sem_create(int value) {
    struct semaphore_t* sem;

    if (value < 0) return ERROR;

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

    hw_irq_enable(0);
    spin_lock(&sem->lock);

    sem->value--;
    if (sem->value < 0) {
        current_task->status = SUSPENDED;
        current_task->current_queue = sem->tasks;
        queue_add(sem->tasks, current_task);
        spin_unlock(&sem->lock);
        hw_irq_enable(1);
        task_switch(&kernel_task);
    } else {
        spin_unlock(&sem->lock);
        hw_irq_enable(1);
    }

    sem = map_get(semaphores, sem_id);
    if (sem == NULL) return ERROR;

    return NOERROR;
}

// libera o acesso a um semáforo
// Retorno: NOERROR (0) ou ERROR (<0)
int sem_up(int sem_id) {
    struct semaphore_t* sem = map_get(semaphores, sem_id);
    if (sem == NULL) return ERROR;

    struct task_t* task = NULL;

    hw_irq_enable(0);
    spin_lock(&sem->lock);
    sem->value++;
    if (sem->value <= 0) {
        task = queue_head(sem->tasks);
        if (task != NULL) task_awake(task);
    }
    spin_unlock(&sem->lock);
    hw_irq_enable(1);

    return NOERROR;
}

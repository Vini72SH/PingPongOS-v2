// PingPongOS - PingPong Operating System

// Gerência de filas de mensagens

#include <string.h>

#include "kernel/macros.h"
#include "kernel/memory.h"
#include "kernel/semaphore.h"
#include "lib/map.h"

struct map_t* queues = NULL;
const int NUM_QUEUES = 32;

struct mqueue_t {
    int size;
    int start;
    int end;
    int current_size;
    int msg_size;
    int s_queue, s_item, s_vaga;
    void* elements;
};

// inicia o subsistema de filas de mensagens
// (chamada pelo núcleo na inicialização).
void mqueue_init() {
    queues = map_create(NUM_QUEUES);
    ppos_debug("subsystem message queue initiated\n");
}

// encerra o subsistema de filas de mensagens
// (chamada pelo núcleo no encerramento).
void mqueue_term() { map_destroy(queues); }

// cria uma fila de mensagens.
// Retorno: descritor da nova fila ou ERROR
int mqueue_create(int max_msgs, int msg_size) {
    struct mqueue_t* mqueue = mem_alloc(sizeof(struct mqueue_t));

    if (mqueue == NULL) return ERROR;

    if (max_msgs <= 0 || msg_size <= 0) return ERROR;

    mqueue->s_queue = sem_create(1);
    if (mqueue->s_queue == ERROR) return ERROR;

    mqueue->s_item = sem_create(0);
    if (mqueue->s_item == ERROR) return ERROR;

    mqueue->s_vaga = sem_create(max_msgs);
    if (mqueue->s_vaga == ERROR) return ERROR;

    mqueue->size = max_msgs;
    mqueue->start = 0;
    mqueue->end = 0;
    mqueue->current_size = 0;
    mqueue->msg_size = msg_size;

    mqueue->elements = mem_alloc(max_msgs * msg_size);
    if (mqueue->elements == NULL) {
        sem_destroy(mqueue->s_vaga);
        sem_destroy(mqueue->s_item);
        sem_destroy(mqueue->s_queue);
        mem_free(mqueue);
        return ERROR;
    }

    int id = map_put(queues, mqueue);

    return id;
}

// destrói uma fila de mensagens, liberando recursos e tarefas
// Retorno: NOERROR ou ERROR
int mqueue_destroy(int mqueue_id) {
    struct mqueue_t* mqueue = map_del(queues, mqueue_id);
    if (mqueue == NULL) return ERROR;

    sem_destroy(mqueue->s_vaga);
    sem_destroy(mqueue->s_item);
    sem_destroy(mqueue->s_queue);
    mem_free(mqueue->elements);
    mem_free(mqueue);

    return NOERROR;
}

// envia uma mensagem
// Retorno: NOERROR ou ERROR
int mqueue_send(int mqueue_id, void* msg) {
    struct mqueue_t* mqueue = map_get(queues, mqueue_id);
    if (mqueue == NULL) return ERROR;

    sem_down(mqueue->s_vaga);

    mqueue = map_get(queues, mqueue_id);
    if (mqueue == NULL) return ERROR;

    sem_down(mqueue->s_queue);

    mqueue = map_get(queues, mqueue_id);
    if (mqueue == NULL) return ERROR;

    memcpy(mqueue->elements + mqueue->end * mqueue->msg_size, msg,
           mqueue->msg_size);
    mqueue->end = (mqueue->end + 1) % mqueue->size;
    mqueue->current_size++;

    sem_up(mqueue->s_queue);

    sem_up(mqueue->s_item);

    return NOERROR;
}

// recebe uma mensagem
// Retorno: NOERROR ou ERROR
int mqueue_recv(int mqueue_id, void* msg) {
    struct mqueue_t* mqueue = map_get(queues, mqueue_id);
    if (mqueue == NULL) return ERROR;

    sem_down(mqueue->s_item);

    mqueue = map_get(queues, mqueue_id);
    if (mqueue == NULL) return ERROR;

    sem_down(mqueue->s_queue);

    mqueue = map_get(queues, mqueue_id);
    if (mqueue == NULL) return ERROR;

    memcpy(msg, mqueue->elements + mqueue->start * mqueue->msg_size,
           mqueue->msg_size);
    mqueue->start = (mqueue->start + 1) % mqueue->size;
    mqueue->current_size--;

    sem_up(mqueue->s_queue);

    sem_up(mqueue->s_vaga);

    return NOERROR;
}

// retorna o numero de mensagens em uma fila
// Retorno: número >= 0 ou ERROR
int mqueue_msgs(int mqueue_id) {
    struct mqueue_t* mqueue = map_get(queues, mqueue_id);
    if (mqueue == NULL) return ERROR;

    return mqueue->current_size;
}

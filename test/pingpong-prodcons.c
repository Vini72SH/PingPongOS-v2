// PingPongOS - PingPong Operating System

#include <assert.h>

#include "lib/pplibc.h"
#include "syscall.h"

struct task_t *p1, *p2, *p3, *c1, *c2;
struct queue_t* buffer;
int s_buffer, s_item, s_vaga;

void producer(void* arg) {
    int stop = 0;
    while (stop == 0) {
        task_sleep(1000);

        int* item = mem_alloc(sizeof(int));
        (*item) = randnum() % 100;

        printk("%5u ms: %s produziu %d (tem %d)\n", time(), task_name(NULL),
               (*item), queue_size(buffer));
        sem_down(s_vaga);

        sem_down(s_buffer);

        queue_add(buffer, item);

        if (queue_size(buffer) == 5) stop = 1;

        sem_up(s_buffer);

        sem_up(s_item);
    }

    task_exit(NOERROR);
}

void consumer(void* arg) {
    int stop = 0;
    while (stop == 0) {
        sem_down(s_item);

        sem_down(s_buffer);

        int* item = queue_head(buffer);
        queue_del(buffer, item);

        sem_up(s_buffer);

        sem_up(s_vaga);

        if (item != NULL) {
            printk(
                "%5u ms:                              %s consumiu %d (tem "
                "%d)\n",
                time(), task_name(NULL), (*item), queue_size(buffer));
            mem_free(item);
        }

        if (queue_size(buffer) == 0) stop = 1;

        task_sleep(1000);
    }

    task_exit(NOERROR);
}

void user_main(void* arg) {
    randseed(0);

    int status, id;
    char* name;

    name = task_name(NULL);
    id = task_id(NULL);
    printk("%5u ms: %s (id %d): inicio\n", time(), name, id);

    // Cria o buffer e os semáforos
    buffer = queue_create();
    assert(buffer != NULL);

    s_buffer = sem_create(1);
    assert(s_buffer >= 0);

    s_item = sem_create(0);
    assert(s_item >= 0);

    s_vaga = sem_create(5);
    assert(s_vaga >= 0);

    // Cria os produtores
    p1 = task_create("p1", producer, NULL);
    assert(p1);
    p2 = task_create("p2", producer, NULL);
    assert(p2);
    p3 = task_create("p3", producer, NULL);
    assert(p3);

    // Cria os consumidores
    c1 = task_create("c1", consumer, NULL);
    assert(c1);
    c2 = task_create("c2", consumer, NULL);
    assert(c2);

    // Aguarda as tarefas encerrarem sem error
    status = task_wait(p1);
    assert(status == NOERROR);
    status = task_wait(p2);
    assert(status == NOERROR);
    status = task_wait(p3);
    assert(status == NOERROR);
    status = task_wait(c1);
    assert(status == NOERROR);
    status = task_wait(c2);
    assert(status == NOERROR);

    // Destrói as tarefas
    status = task_destroy(p1);
    assert(status == NOERROR);
    status = task_destroy(p2);
    assert(status == NOERROR);
    status = task_destroy(p3);
    assert(status == NOERROR);
    status = task_destroy(c1);
    assert(status == NOERROR);
    status = task_destroy(c2);
    assert(status == NOERROR);

    // Limpa o buffer
    void* aux = queue_head(buffer);
    while (aux != NULL) {
        queue_del(buffer, aux);
        mem_free(aux);
        aux = queue_head(buffer);
    }
    queue_destroy(buffer);

    // Destrói as estruturas de dados
    status = sem_destroy(s_vaga);
    assert(status == NOERROR);
    status = sem_destroy(s_item);
    assert(status == NOERROR);
    status = sem_destroy(s_buffer);
    assert(status == NOERROR);

    task_exit(NOERROR);
}

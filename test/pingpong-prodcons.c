// PingPongOS - PingPong Operating System

#include <assert.h>

#include "lib/pplibc.h"
#include "syscall.h"

struct task_t *p1, *p2, *p3, *c1, *c2;
struct queue_t* buffer;
int s_buffer, s_item, s_vaga;

void producer(void* arg) {
    while (true) {
        task_sleep(1000);

        int* item = mem_alloc(sizeof(int));
        (*item) = randnum() % 100;

        printk("%5u ms: %s produziu %d\n", time(), task_name(NULL), (*item));
        sem_down(s_vaga);

        sem_down(s_buffer);

        queue_add(buffer, item);

        sem_up(s_buffer);

        sem_up(s_item);
    }
}

void consumer(void* arg) {
    while (true) {
        sem_down(s_item);

        sem_down(s_buffer);

        int* item = queue_head(buffer);
        queue_del(buffer, item);

        sem_up(s_buffer);

        sem_up(s_vaga);

        printk("%5u ms: %s consumiu %d\n", time(), task_name(NULL), (*item));

        task_sleep(1000);
    }
}

void user_main(void* arg) {
    randseed(0);

    int status, id;
    char* name;

    name = task_name(NULL);
    id = task_id(NULL);
    printk("%5u ms: %s (id %d): inicio\n", time(), name, id);

    buffer = queue_create();
    assert(buffer != NULL);

    s_buffer = sem_create(1);
    assert(s_buffer >= 0);

    s_item = sem_create(1);
    assert(s_item >= 0);

    s_vaga = sem_create(1);
    assert(s_vaga >= 0);

    p1 = task_create("p1", producer, NULL);
    assert(p1);

    p2 = task_create("p2", producer, NULL);
    assert(p2);

    p3 = task_create("p3", producer, NULL);
    assert(p3);

    c1 = task_create("c1", consumer, NULL);
    assert(c1);

    c2 = task_create("c2", consumer, NULL);
    assert(c2);

    queue_destroy(buffer);

    status = sem_destroy(s_vaga);
    assert(status == NOERROR);

    status = sem_destroy(s_item);
    assert(status == NOERROR);

    status = sem_destroy(s_buffer);
    assert(status == NOERROR);

    task_exit(NOERROR);
}

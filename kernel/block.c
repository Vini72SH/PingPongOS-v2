// PingPongOS - PingPong Operating System

// Gerência de um dispositivo orientado a blocos.

#include "hardware/cpu.h"
#include "hardware/disk.h"
#include "kernel/dispatcher.h"
#include "kernel/macros.h"
#include "kernel/memory.h"
#include "kernel/semaphore.h"
#include "kernel/task.h"
#include "kernel/tcb.h"
#include "lib/queue.h"

#define READY 0
#define WAITING 1
#define FINISHED 2
#define FAIL 3

#define READ 0
#define WRITE 1

struct request_t {
    int task_id;
    int status;
    int operation;
    int block;
    void* buffer;
};

struct task_t* disk_manager;
struct queue_t* requests;
struct queue_t* waiting;
extern struct queue_t* suspended_tasks;

extern struct task_t* current_task;

int irq = 0;
int sem_queue;

void manager(void* arg);

void disk_handler(int i) {
    irq = 1;
    task_awake(disk_manager);
}

// inicia o subsistema de gestão do disco virtual armazenado em "disk_image"
// (chamada pelo núcleo na inicialização).
void block_init(char* disk_image) {
    int status = hw_disk(DISK_CMD_INIT, 0, disk_image);
    if (status == ERROR) {
        ppos_debug("Erro ao inicializar o disco\n");
        return;
    }

    disk_manager = task_create("disk_manager", manager, NULL);
    if (disk_manager == NULL) {
        ppos_debug("Erro ao criar a tarefa gerenciadora do disco\n");
        return;
    }

    requests = queue_create();
    if (requests == NULL) {
        ppos_debug("Erro ao criar a fila de requisições\n");
        return;
    }

    waiting = queue_create();
    if (waiting == NULL) {
        ppos_debug("Erro ao criar a fila de tarefas esperando\n");
        return;
    }

    sem_queue = sem_create(1);
    if (sem_queue < 0) {
        ppos_debug("Erro ao criar o semáforo\n");
        return;
    }

    hw_irq_handle(IRQ_DISK, disk_handler);
}

// encerra o subsistema de gestão do disco virtual
// (chamada pelo núcleo no encerramento).
void block_term(char* disk_image) {
    if (sem_queue >= 0) sem_destroy(sem_queue);
    if (requests) queue_destroy(requests);
    if (waiting) queue_destroy(waiting);
    task_destroy(disk_manager);
}

// retorna o tamanho de cada bloco do disco, em bytes
int block_size() {
    int status = hw_disk(DISK_CMD_BLOCKSIZE, 0, NULL);
    if (status < 0) {
        ppos_debug("Houve um erro ao ler o tamanho do bloco\n");
    }

    return status;
}

// retorna o tamanho do disco, em blocos
int block_blocks() {
    int status = hw_disk(DISK_CMD_DISKSIZE, 0, NULL);
    if (status < 0) {
        ppos_debug("Houve um erro ao obter o tamanho do disco\n");
    }

    return status;
}

// leitura de um bloco, do disco para o buffer
int block_read(int block, void* buffer) {
    struct request_t* req = mem_alloc(sizeof(struct request_t));

    req->task_id = task_id(NULL);
    req->status = READY;
    req->operation = READ;
    req->block = block;
    req->buffer = buffer;

    sem_down(sem_queue);
    queue_add(requests, req);
    queue_add(waiting, current_task);
    queue_add(suspended_tasks, current_task);
    sem_up(sem_queue);

    task_awake(disk_manager);
    task_suspend(NULL);

    int status = req->status;
    mem_free(req);

    if (status == FAIL) return ERROR;

    return NOERROR;
}

// escrita de um bloco, do buffer para o disco
int block_write(int block, void* buffer) {
    struct request_t* req = mem_alloc(sizeof(struct request_t));

    req->task_id = task_id(NULL);
    req->status = READY;
    req->operation = WRITE;
    req->block = block;
    req->buffer = buffer;

    sem_down(sem_queue);
    queue_add(requests, req);
    queue_add(waiting, current_task);
    queue_add(suspended_tasks, current_task);
    sem_up(sem_queue);

    task_awake(disk_manager);
    task_suspend(NULL);

    int status = req->status;
    mem_free(req);

    if (status == FAIL) return ERROR;

    return NOERROR;
}

void manager(void* arg) {
    struct task_t* task;
    struct request_t* req;
    while (1) {
        if (irq) {
            irq = 0;
            sem_down(sem_queue);
            req = queue_head(requests);
            task = queue_head(waiting);

            queue_del(suspended_tasks, task);
            queue_del(waiting, task);
            queue_del(requests, req);
            sem_up(sem_queue);

            if (task && req) {
                req->status = FINISHED;
                task_awake(task);
            }
        }

        if (hw_disk(DISK_CMD_STATUS, 0, NULL) == DISK_STATUS_IDLE) {
            sem_down(sem_queue);
            req = queue_head(requests);
            while (req != NULL) {
                if (req->status == READY) break;
                req = queue_next(requests);
            }
            sem_up(sem_queue);

            if (req) {
                req->status = WAITING;
                int op = (req->operation == READ) ? (DISK_CMD_READ)
                                                  : (DISK_CMD_WRITE);
                hw_disk(op, req->block, req->buffer);
            }
        }

        task_suspend(NULL);
    }
}
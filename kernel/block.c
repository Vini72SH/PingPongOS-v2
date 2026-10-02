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
    struct task_t* task;
};

struct task_t* disk_manager;
struct queue_t* requests;
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
    req->task = current_task;

    sem_down(sem_queue);
    queue_add(requests, req);
    queue_add(suspended_tasks, req->task);
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
    req->task = current_task;

    sem_down(sem_queue);
    queue_add(requests, req);
    queue_add(suspended_tasks, req->task);
    sem_up(sem_queue);

    task_awake(disk_manager);
    task_suspend(NULL);

    int status = req->status;
    mem_free(req);

    if (status == FAIL) return ERROR;

    return NOERROR;
}

void manager(void* arg) {
    struct request_t* current_req = NULL;
    while (1) {
        if (irq) {
            irq = 0;
            if (current_req) {
                sem_down(sem_queue);
                queue_del(suspended_tasks, current_req->task);
                sem_up(sem_queue);
                current_req->status = FINISHED;
                task_awake(current_req->task);
                current_req = NULL;
            }
        }

        if (hw_disk(DISK_CMD_STATUS, 0, NULL) == DISK_STATUS_IDLE) {
            sem_down(sem_queue);
            current_req = queue_head(requests);
            while (current_req != NULL) {
                if (current_req->status == READY) break;
                current_req = queue_next(requests);
            }
            queue_del(requests, current_req);
            sem_up(sem_queue);

            if (current_req) {
                current_req->status = WAITING;
                int op = (current_req->operation == READ) ? (DISK_CMD_READ)
                                                          : (DISK_CMD_WRITE);
                hw_disk(op, current_req->block, current_req->buffer);
            }
        }

        task_suspend(NULL);
    }
}
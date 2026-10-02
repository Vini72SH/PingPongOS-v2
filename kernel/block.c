// PingPongOS - PingPong Operating System

// Gerência de um dispositivo orientado a blocos.

#include "kernel/task.h"
#include "kernel/tcb.h"

#define READ 0
#define WRITE 1

struct req {
    int task_id;
    int status;
};

struct task_t* disk_manager;
struct queue_t* requests;

void manager(void*);

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
}

// encerra o subsistema de gestão do disco virtual
// (chamada pelo núcleo no encerramento).
void block_term(char* disk_image) { task_destroy(disk_manager); }

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
int block_read(int block, void* buffer) {}

// escrita de um bloco, do buffer para o disco
int block_write(int block, void* buffer) {}

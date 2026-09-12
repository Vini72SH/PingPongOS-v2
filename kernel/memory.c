// PingPongOS - PingPong Operating System

// Este arquivo PODE/DEVE ser alterado.

// Alocador básico de memória heap.

// somente para a implementação trivial
#include <string.h>

#include "lib/pplibc.h"

#define ERROR -1
#define NOERROR 0

#define HEAP_SIZE 64 * 1024 * 1024  // 64MB
#define ALIGNMENT 16

static char heap[HEAP_SIZE];

typedef struct heap_block heap_block;

struct heap_block {
    unsigned short free;  // (2B)
    unsigned short id;    // (2B)
    unsigned int size;    // (4B)
    heap_block* prev;     // (8B)
    heap_block* next;     // (8B)
    char* ptr;            // (8B)
};

unsigned short bid = 0;

unsigned int free_blocks = 0;
unsigned int allocated_blocks = 0;
unsigned int current_space = HEAP_SIZE;
unsigned int allocated_space = 0;

// inicia o subsistema de memória RAM (heap)
// (chamada pelo núcleo na inicialização).
void mem_init() {
    heap_block initial_block;

    initial_block.free = 1;
    initial_block.id = bid++;
    initial_block.size = HEAP_SIZE - sizeof(heap_block);
    initial_block.prev = NULL;
    initial_block.next = NULL;
    initial_block.ptr = (char*)heap + (sizeof(heap_block));

    free_blocks = 1;
    allocated_blocks = 0;
    current_space = HEAP_SIZE - sizeof(heap_block);
    allocated_space = 0;

    memcpy(&heap, &initial_block, sizeof(heap_block));

    printk("Sistema Inicializado\n");
    printk("Tamanho da HEAP: %d\n", HEAP_SIZE);
}

// encerra o subsistema de memória RAM (heap)
// (chamada pelo núcleo no encerramento).
void mem_term() {}

// informa a quantidade de memória total, em bytes
int mem_size() { return HEAP_SIZE; }

// informa a quantidade de memória disponível, em bytes
int mem_avail() { return current_space; }

// aloca um bloco de memória com o tamanho indicado
// retorna ponteiro ou NULL se houver erro
void* mem_alloc(int size) {
    if (size <= 0) {
        printk("Tamanho <= 0: Retornando NULL\n");
        return NULL;
    }

    unsigned int block_size = size;

    if (current_space < block_size) {
        printk("Tamanho atual insuficiente: Retornando NULL\n\n");
        return NULL;
    }

    while (block_size % 16) block_size++;

    // printk("Alocação de um bloco de %d + %d (%d) bytes\n", size,
    //        (size % ALIGNMENT), block_size);

    heap_block* block = (heap_block*)heap;
    while (block != NULL) {
        if (block->free && block->size >= block_size) break;
        block = block->next;
    }

    if (block == NULL) {
        printk("Não há blocos livres disponíveis: Retornando NULL\n\n");
        return NULL;
    }

    // printk("Bloco de Memória alocado!\n");
    // printk("Endereço: %p | Ponteiro %p | Tamanho %d\n",
    //        (void*)block - (void*)heap, (void*)block->ptr - (void*)heap,
    //        block_size);

    block->free = 0;
    if (block->size >= block_size + (sizeof(heap_block)) + ALIGNMENT) {
        heap_block* next;

        next = (heap_block*)((char*)block->ptr + block_size);
        next->id = bid++;
        next->free = 1;
        next->prev = block;
        next->next = block->next;
        next->size = block->size - (block_size + sizeof(heap_block));
        next->ptr = (void*)((char*)next + (sizeof(heap_block)));

        block->size = block_size;
        block->next = next;

        // printk("Novo bloco livre criado!\n");
        // printk("Endereço: %p | Ponteiro %p | Tamanho %d\n",
        //        (void*)next - (void*)heap, (void*)next->ptr - (void*)heap,
        //        next->size);

        current_space -= sizeof(heap_block);
        free_blocks++;
    }

    current_space -= block->size;
    allocated_space += block->size;
    allocated_blocks++;
    free_blocks--;

    // printk("Bloco alocado com sucesso!\n\n");

    return block->ptr;
}

// libera um bloco de memória previamente alocado
// retorna NOERROR se ok ou ERROR se ptr for NULL ou inválido
int mem_free(void* ptr) {
    if (ptr == NULL) return ERROR;

    heap_block* block = (heap_block*)heap;
    while (block != NULL) {
        if (block->ptr == ptr) break;
        block = block->next;
    }

    if (block == NULL) return ERROR;
    block->free = 1;

    free_blocks++;
    allocated_blocks--;
    allocated_space -= block->size;
    current_space += block->size;

    return (NOERROR);
}

// gera um relatório sobre o uso da memória
void mem_report() {
    printk("heap: %d KB allocated (%d blocks), %d KB free (%d blocks)\n",
           (allocated_space / 1000), allocated_blocks, (current_space / 1000),
           free_blocks);

    heap_block* block = (heap_block*)heap;
    while (block != NULL) {
        char* s = (block->free) ? ("free") : ("aloc");
        printk("heap: block %5d: %p - %p %s prev %5d next %5d size %5d\n",
               block->id, block->ptr, block->ptr + block->size, s,
               (block->prev == NULL) ? (0) : (block->prev->id),
               (block->next == NULL) ? (0) : (block->next->id), block->size);
        block = block->next;
    }
}

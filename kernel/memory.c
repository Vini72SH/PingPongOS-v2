// PingPongOS - PingPong Operating System

// Este arquivo PODE/DEVE ser alterado.

// Alocador básico de memória heap.

// somente para a implementação trivial
#include <stdlib.h>

#define NOERROR 0

#define HEAP_SIZE 64 * 1024 * 1024

static unsigned char heap[HEAP_SIZE];

struct heap_block {
    unsigned int id;
    unsigned int size;
    char isFree;
    unsigned int prev;
    unsigned int next;
};

// inicia o subsistema de memória RAM (heap)
// (chamada pelo núcleo na inicialização).
void mem_init();

// encerra o subsistema de memória RAM (heap)
// (chamada pelo núcleo no encerramento).
void mem_term();

// informa a quantidade de memória total, em bytes
int mem_size();

// informa a quantidade de memória disponível, em bytes
int mem_avail();

// aloca um bloco de memória com o tamanho indicado
// retorna ponteiro ou NULL se houver erro
void* mem_alloc(int size) { return (malloc(size)); }

// libera um bloco de memória previamente alocado
// retorna NOERROR se ok ou ERROR se ptr for NULL ou inválido
int mem_free(void* ptr) {
    if (ptr == NULL) return ERROR;
    free(ptr);
    return (NOERROR);
}

// gera um relatório sobre o uso da memória
void mem_report();

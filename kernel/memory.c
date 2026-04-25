// PingPongOS - PingPong Operating System

// Este arquivo PODE/DEVE ser alterado.

// Alocador básico de memória heap.

#include <stdlib.h>

void mem_init() {}

void* mem_alloc(int size) { return (malloc(size)); }

int mem_free(void* ptr) {
    free(ptr);
    return (0);
}

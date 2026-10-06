// PingPongOS - PingPong Operating System

// Gerência de uma cache de blocos

#include "kernel/cache.h"

#include <string.h>

#include "kernel/block.h"
#include "kernel/macros.h"
#include "kernel/memory.h"
#include "kernel/semaphore.h"
#include "kernel/time.h"

#define CACHE_BLOCK_SIZE 64
#define NUM_CACHE_BLOCKS 64

int sem_cache = -1;

struct cache_block_t {
    int block_id;
    int last_access;
    char buffer[CACHE_BLOCK_SIZE];
};

struct cache_t {
    struct cache_block_t* blocks;
};

struct cache_t* cache;

// inicia o subsistema de gestão da cache de blocos
// (chamada pelo núcleo na inicialização).
void cache_init() {
    cache = mem_alloc(sizeof(struct cache_t));
    if (cache == NULL) {
        ppos_debug("Não foi possível alocar a cache\n");
        return;
    }

    cache->blocks = mem_alloc(NUM_CACHE_BLOCKS * sizeof(struct cache_block_t));
    if (cache->blocks == NULL) {
        ppos_debug("Não foi possível alocar os blocos da cache\n");
        return;
    }

    for (int i = 0; i < NUM_CACHE_BLOCKS; i++) {
        cache->blocks[i].block_id = -1;
        cache->blocks[i].last_access = 0;
    }

    sem_cache = sem_create(1);
    if (sem_cache < 0) {
        ppos_debug("Erro ao criar o semáforo da cache\n");
        return;
    }
}

// encerra o subsistema de gestão da cache
// (chamada pelo núcleo no encerramento).
void cache_term() {
    if (cache != NULL) mem_free(cache);
}

// leitura de um bloco, da cache para o buffer
int cache_read(int block, void* buffer) {
    int cdx = -1;
    int last_access = time() + 1;
    int find = 0;

    sem_down(sem_cache);

    for (int i = 0; i < NUM_CACHE_BLOCKS; i++) {
        // Se encontrou o bloco procurado, interrompe o loop
        if (cache->blocks[i].block_id == block) {
            cdx = i;
            find = 1;
            break;
        } else {
            // Busca o bloco menos recentemente utilizado
            if (cache->blocks[i].last_access < last_access) {
                cdx = i;
                last_access = cache->blocks[i].last_access;
            }
        }
    }

    if (find == 0) {
        int status = block_read(block, cache->blocks[cdx].buffer);
        if (status != 0) {
            sem_up(sem_cache);
            return status;
        }

        cache->blocks[cdx].block_id = block;
    }

    memcpy(buffer, cache->blocks[cdx].buffer, CACHE_BLOCK_SIZE);
    cache->blocks[cdx].last_access = time();

    sem_up(sem_cache);

    return 0;
}

// escrita de um bloco, do buffer para a cache
int cache_write(int block, void* buffer) {
    int cdx = -1;
    int last_access = time() + 1;
    int find = 0;

    sem_down(sem_cache);

    for (int i = 0; i < NUM_CACHE_BLOCKS; i++) {
        // Se encontrou o bloco procurado, interrompe o loop
        if (cache->blocks[i].block_id == block) {
            cdx = i;
            find = 1;
            break;
        } else {
            // Busca o bloco menos recentemente utilizado
            if (cache->blocks[i].last_access < last_access) {
                cdx = i;
                last_access = cache->blocks[i].last_access;
            }
        }
    }

    if (find == 0) {
        cache->blocks[cdx].block_id = block;
    }

    memcpy(cache->blocks[cdx].buffer, buffer, CACHE_BLOCK_SIZE);
    cache->blocks[cdx].last_access = time();
    int status = block_write(block, buffer);

    sem_up(sem_cache);

    return status;
}

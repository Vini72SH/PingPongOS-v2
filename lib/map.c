// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 2.1 -- 07/2026

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

struct map_t {
    int size;
    int ptr;
    int regs;
    void** elements;
};

// Cria um mapa para até N objetos, com IDs entre 0 e N-1.
// Retorno: ponteiro para o mapa ou NULL (erro)
struct map_t* map_create(int size) {
    if (size <= 0) return NULL;

    struct map_t* map = malloc(sizeof(struct map_t));
    if (map == NULL) return NULL;

    map->size = size;
    map->ptr = 0;
    map->regs = 0;
    map->elements = calloc(size, sizeof(void*));
    if (map->elements == NULL) {
        free(map);
        return NULL;
    }

    return map;
}

// destrói um mapa existente (mas não destrói os objetos).
// Retorno: 0 em sucesso ou -1 (erro)
int map_destroy(struct map_t* map) {
    if (map == NULL) return -1;

    map->size = 0;
    map->ptr = 0;
    map->regs = 0;
    free(map->elements);
    free(map);

    return 0;
}

// Registra um objeto no mapa, retornando seu ID.
// Retorno: ID atribuído ao objeto ou -1 (erro).
int map_put(struct map_t* map, void* object) {
    if ((map == NULL) || (object == NULL)) return -1;

    int aux = map->ptr;
    do {
        if (map->elements[aux] == NULL) break;
        aux = (aux + 1) % map->size;
    } while (aux != map->ptr);

    if (map->elements[aux] != NULL) return -1;

    map->regs++;
    map->elements[aux] = object;

    return aux;
}

// Informa o objeto registrado no ID indicado do mapa.
// Retorno: ponteiro para o objeto ou NULL (erro)
void* map_get(struct map_t* map, int id) {
    if (map == NULL) return NULL;
    if ((id < 0) || (id >= map->size)) return NULL;

    return map->elements[id];
}

// Libera um ID do mapa e devolve o objeto associado.
// Retorno: ponteiro para o objeto ou NULL (erro)
void* map_del(struct map_t* map, int id) {
    if (map == NULL) return NULL;
    if ((id < 0) || (id >= map->size)) return NULL;

    void* obj = map->elements[id];
    map->elements[id] = NULL;
    map->regs--;

    return obj;
}

// Informa o número de objetos registrados no mapa.
// Retorno: número de objetos registrados ou -1 (erro)
int map_items(struct map_t* map) {
    if (map == NULL) return -1;

    return map->regs;
}

// Informa o número de objetos que o mapa pode registrar.
// Retorno: número de objetos ou -1 (erro)
int map_size(struct map_t* map) {
    if (map == NULL) return -1;

    return map->size;
}

// Imprime o conteúdo do mapa, no seguinte formato:
// Mapa nulo:     nome: undef
// Mapa vazio:    nome: [ - - - - - ] (0/5)
// Mapa qualquer: nome: [ - * - * * ] (3/5)
// As posições "*" no vetor impresso correspondem aos IDs em uso no mapa;
// (3/5) indica que o mapa tem 3 objetos e pode registrar até 5 objetos.
void map_print(char* name, struct map_t* map) {
    if (map == NULL) {
        printf("%s: undef\n", name);
        return;
    }

    printf("%s: ", name);
    printf("[ ");
    for (int i = 0; i < map->size; i++) {
        char c = (map->elements[i] == NULL) ? ('-') : ('*');
        printf("%c ", c);
    }
    printf("] ");
    printf("(%d/%d)\n", map->regs, map->size);
}

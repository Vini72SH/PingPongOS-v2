// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 2.0 -- Junho de 2025

#include "queue.h"

#include "kernel/memory.h"
#include "libc.h"

typedef struct node_t node_t;

struct node_t {
    void* element;
    node_t* next;
};

struct queue_t {
    int size;
    node_t* it;
    node_t* start;
    node_t* end;
};

// Cria uma fila inicialmente vazia.
// Retorno: ponteiro p/ a nova fila
//          NULL se houver erro
struct queue_t* queue_create() {
    struct queue_t* queue;

    queue = mem_alloc(sizeof(struct queue_t));
    if (queue == NULL) return NULL;

    queue->size = 0;
    queue->it = NULL;
    queue->start = NULL;
    queue->end = NULL;

    return queue;
}

// Destroi uma fila, liberando a memória alocada por ela.
// IMPORTANTE: os itens apontados pela fila NÃO devem ser liberados,
// pois a aplicação que os criou e pôs na fila é responsável por eles.
// Retorno: NOERROR ou ERROR (se a fila não existir)
int queue_destroy(struct queue_t* queue) {
    if (queue == NULL) return ERROR;

    node_t* aux = queue->start;
    node_t* next = NULL;

    while (aux != NULL) {
        next = aux->next;
        mem_free(aux);
        aux = next;
    }

    mem_free(queue);
    queue = NULL;

    return NOERROR;
}

// Adiciona um item no fim da fila; ajusta o iterador para ele
// se for o primeiro item (ou seja, se a fila estiver vazia).
// Retorno: NOERROR ou ERROR (se fila ou item não existir)
int queue_add(struct queue_t* queue, void* item) {
    if ((queue == NULL) || (item == NULL)) return ERROR;

    node_t* node = mem_alloc(sizeof(node_t));
    if (node == NULL) return ERROR;

    node->element = item;
    node->next = NULL;

    if ((queue->size == 0) || (queue->start == NULL) || (queue->end == NULL)) {
        queue->start = node;
        queue->end = node;
        queue->it = node;
    } else {
        queue->end->next = node;
        queue->end = node;
    }

    queue->size++;

    return NOERROR;
}

// Retira da fila o item com o valor indicado; se o item estiver
// em mais de uma posição da fila, retira apenas da primeira posição
// encontrada; se o item estiver apontado pelo iterador, este avança
// para o próximo item da fila (ou para NULL, se for o último).
// Retorno: NOERROR ou ERROR (não encontrou ou outro erro).
int queue_del(struct queue_t* queue, void* item) {
    if ((queue == NULL) || (item == NULL)) return ERROR;

    node_t* prev = NULL;
    node_t* aux = queue->start;

    while ((aux != NULL) && (aux->element != item)) {
        prev = aux;
        aux = aux->next;
    }

    if (aux == NULL) return ERROR;

    if (queue->it == aux) queue->it = queue->it->next;

    if ((prev == NULL) && (aux->next == NULL)) {
        queue->start = NULL;
        queue->end = NULL;
        queue->it = NULL;
    } else if (prev == NULL)
        queue->start = aux->next;
    else
        prev->next = aux->next;

    if (aux->next == NULL) queue->end = prev;

    queue->size--;
    mem_free(aux);

    return NOERROR;
}

// Informa se o item indicado está na fila.
// Retorno: true/false (error: false).
bool queue_has(struct queue_t* queue, void* item) {
    if ((queue == NULL) || (item == NULL)) return false;

    node_t* aux = queue->start;

    while ((aux != NULL) && (aux->element != item)) {
        aux = aux->next;
    }

    if (aux == NULL) return false;

    return true;
}

// Informa o número de itens na fila.
// Retorno: número de itens na fila (>= 0)
//          ERROR se a fila não existir
int queue_size(struct queue_t* queue) {
    if (queue == NULL) return ERROR;

    return queue->size;
}

// Põe o iterador no início da fila.
// Retorno: ptr para o item apontado pelo iterador
//          NULL se a fila estiver vazia ou não existir
void* queue_head(struct queue_t* queue) {
    if ((queue == NULL) || queue->start == NULL) return NULL;

    queue->it = queue->start;

    return queue->it->element;
}

// Avança o iterador ao próximo item na fila.
// Retorno: ptr para o item apontado pelo iterador após avançar
//          NULL se o iterador passou do último item da fila
//          NULL se a fila estiver vazia ou não existir
void* queue_next(struct queue_t* queue) {
    if ((queue == NULL) || (queue->it == NULL)) return NULL;

    queue->it = queue->it->next;
    if (queue->it == NULL) return NULL;

    return queue->it->element;
}

// Informa o item atualmente sob o iterador na fila.
// Retorno: ptr para o item apontado pelo iterador
//          NULL se a fila estiver vazia ou não existir
//          NULL se o iterador passou do fim da fila
void* queue_item(struct queue_t* queue) {
    if ((queue == NULL) || (queue->start == NULL) || (queue->it == NULL))
        return NULL;

    return queue->it->element;
}

// Imprime os elementos de uma fila; a função externa "func"
// deve ser chamada para imprimir cada item.
// Exemplos de saída, com name == "Frutas":
// Frutas: [ banana pera ameixa uva ] (4 items)
// Frutas: [ ] (0 items)
// Frutas: undef   se queue == NULL
// Frutas: [ undef undef undef ] (3 items)  se func == NULL
void queue_print(char* name, struct queue_t* queue, void(func)(void*)) {
    if (queue == NULL) {
        printk("%s: undef\n", name);
        return;
    }

    printk("%s: [ ", name);
    node_t* aux = queue->start;
    while (aux != NULL) {
        if (func == NULL)
            printk("undef");
        else
            func(aux->element);
        printk(" ");
        aux = aux->next;
    }
    printk("] (%d items)\n", queue->size);
}
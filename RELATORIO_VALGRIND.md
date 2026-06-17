# Relatório do erro no pingpong-scheduler sob Valgrind

## Resumo

O erro que aparecia ao executar pingpong-scheduler sob Valgrind não era um defeito real na estrutura de fila em si. O que acontecia era corrupção de memória causada pelo tamanho insuficiente da pilha das tarefas. Sem Valgrind, o programa podia parecer estável porque o layout de memória mudava e a falha ficava latente. Com Valgrind, a instrumentação alterava o uso de memória e expunha o problema de forma determinística.

## Como o problema aparecia

O primeiro sintoma era um aviso de acesso inválido dentro de queue_add, no trecho onde a fila insere um novo nó quando já existe pelo menos um elemento. O Valgrind mostrava leituras e escritas inválidas sobre o descritor da fila, com endereços dentro de um bloco de 32 bytes alocado por queue_create. Isso fazia parecer que o ponteiro da cauda da fila estava corrompido.

Na prática, o endereço que o Valgrind apontava era o objeto queue_t inteiro, e não um nó isolado da fila. Isso indicava que o problema vinha antes da inserção, isto é, algum outro código havia bagunçado a memória do descritor da fila ou estruturas adjacentes.

## Causa real

A causa raiz estava no tamanho da pilha das tarefas em [kernel/task.c](kernel/task.c#L10). A pilha definida como 4096 bytes era pequena demais para o cenário executado sob Valgrind. Como cada tarefa usa seu próprio contexto e sua própria pilha, a instrumentação do Valgrind aumentava o consumo de memória e alterava o posicionamento dos blocos alocados. Com isso, a pilha podia invadir áreas vizinhas do heap ou expor uma sobreposição que normalmente não aparecia sem Valgrind.

Quando isso ocorria, o descritor da fila pronta, criado em [lib/queue.c](lib/queue.c#L25) e manipulado em [lib/queue.c](lib/queue.c#L66), passava a conter dados inconsistentes. Na próxima chamada de queue_add, o código tentava encadear um novo nó usando queue->end->next, mas queue->end já não apontava para um nó válido. Por isso o Valgrind reportava leitura e escrita fora dos limites.

## Como eu detectei

Primeiro, comparei o backtrace do Valgrind com o fluxo normal do programa. O erro sempre surgia em queue_add, mas era disparado por task_create, chamada durante a criação das tarefas do teste. Isso já sugeria que a fila estava recebendo um estado ruim vindo de fora.

Depois, conferi o estado da fila com depuração e vi que o descritor da fila de prontas era criado corretamente em dispatcher_init. O fato de o problema aparecer logo após a criação das primeiras tarefas, e não em operações posteriores de remoção, apontava para corrupção de memória causada durante a inicialização do contexto das tarefas, não por uma operação lógica da fila.

A confirmação veio quando aumentei a pilha das tarefas e rodei novamente com Valgrind. O programa passou a executar até o fim sem erros, o que fechou a hipótese de corrupção causada por falta de espaço na stack.

## Ajustes feitos

Foram aplicados três ajustes:

1. A pilha das tarefas foi aumentada para 16384 bytes em [kernel/task.c](kernel/task.c#L10).
2. task_create passou a liberar recursos e abortar corretamente se ctx_create ou queue_add falharem, evitando descritores parcialmente inicializados em [kernel/task.c](kernel/task.c#L49).
3. queue_add e queue_del ficaram mais defensivas em [lib/queue.c](lib/queue.c#L66) e [lib/queue.c](lib/queue.c#L94), tratando melhor fila vazia e remoção do último elemento.

## Validação

O teste final foi:

make pingpong-scheduler && valgrind ./pingpong-scheduler

O resultado final foi limpo: zero erros de memória e nenhuma leak reportada pelo Valgrind.

## Conclusão

O sintoma parecia um bug na fila, mas era efeito colateral de corrupção de memória causada pela pilha pequena das tarefas. O Valgrind foi essencial para revelar o problema porque mudou o layout de memória e tornou a falha reproduzível. Depois do aumento da stack e das proteções adicionais, pingpong-scheduler passou a rodar corretamente sob Valgrind.
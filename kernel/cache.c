// PingPongOS - PingPong Operating System

// Gerência de uma cache de blocos

// inicia o subsistema de gestão da cache de blocos
// (chamada pelo núcleo na inicialização).
void cache_init() {}

// encerra o subsistema de gestão da cache
// (chamada pelo núcleo no encerramento).
void cache_term() {}

// leitura de um bloco, da cache para o buffer
int cache_read(int block, void* buffer) { return 0; }

// escrita de um bloco, do buffer para a cache
int cache_write(int block, void* buffer) { return 0; }

// PingPongOS - PingPong Operating System
// © Prof. Carlos A. Maziero, DINF UFPR
// Versão 2.1 -- 06/2026

// Gerência de uma cache de blocos

#ifndef __PPOS_CACHE__
#define __PPOS_CACHE__

// inicia o subsistema de gestão da cache de blocos
// (chamada pelo núcleo na inicialização).
void cache_init();

// encerra o subsistema de gestão da cache
// (chamada pelo núcleo no encerramento).
void cache_term();

// leitura de um bloco, da cache para o buffer
int cache_read(int block, void* buffer);

// escrita de um bloco, do buffer para a cache
int cache_write(int block, void* buffer);

#endif

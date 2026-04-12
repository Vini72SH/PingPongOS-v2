# TAD filas genéricas
# Prof. Carlos A. Maziero, DINF UFPR
# Versão 1.0 - 09/2025

# ATENÇÃO: ESTE ARQUIVO NÃO DEVE SER ALTERADO;
# ALTERAÇÕES SERÃO DESCARTADAS NA CORREÇÃO.

# flags de compilação e ligação
CC       = gcc
CFLAGS   = -std=c99 -Wall -Wextra -O0 -g -Wno-unused-function -Wno-unused-parameter
LDFLAGS  = -z noexecstack
BIN      = testa-fila
OUT      = saida.txt

# estes alvos não são arquivos
.PHONY: test clean purge

# compila e liga o projeto
testa-fila:   testa-fila.o queue.o
queue.o:      queue.c queue.h
testa-fila.o: testa-fila.c queue.h

# teste: compara saída gerada com saída esperada
test: $(BIN)
	@echo "Gerando saída em $(OUT)"
	./$(BIN) > $(OUT)
	@echo "Comparando saídas esperada e gerada (formato diff)"
	@if diff testa-fila.txt $(OUT) ; then \
	echo "CORRETO: saidas identicas" ; \
	else \
	echo "ERRADO: saidas diferentes" ; \
	fi

# limpa arquivos temporários
clean:
	-rm -f *.o *~ $(OUT)

# limpa tudo, deixa só o o código-fonte
purge: clean
	-rm -f $(BIN)

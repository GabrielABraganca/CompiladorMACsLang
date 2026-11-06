# =============================================================================
# Makefile do Compilador MACSLang
# =============================================================================
#
# Comandos disponíveis:
#   make          - Compila o compilador
#   make clean    - Remove os arquivos gerados
#   make test1    - Compila e executa o teste 1 (fatorial)
#   make test2    - Compila e executa o teste 2 (par/impar)
#   make test3    - Compila e executa o teste 3 (múltiplas funções)
#   make all_tests - Executa todos os testes
#
# =============================================================================

CC     = gcc
CFLAGS = -Wall -Wextra -g -std=c99
SRC    = src/main.c src/analisadorLexico.c src/analisadorSintatico.c \
         src/analisadorSemantico.c src/geradorCodigo.c
OUT    = compilador

# Alvo padrão: compila o compilador
all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC)
	@echo "Compilador gerado: ./$(OUT)"

# Cria a pasta de saída se não existir
output:
	mkdir -p output

# Testes
test1: $(OUT) output
	@echo "\n=== TESTE 1: Fatorial ==="
	./$(OUT) testes/teste1.mac --verbose

test2: $(OUT) output
	@echo "\n=== TESTE 2: Par/Impar e While ==="
	./$(OUT) testes/teste2.mac --ast

test3: $(OUT) output
	@echo "\n=== TESTE 3: Múltiplas Funções ==="
	./$(OUT) testes/teste3.mac --simbolos

all_tests: test1 test2 test3

clean:
	rm -f $(OUT) output/*.asm *.asm
	@echo "Limpeza concluida"

.PHONY: all clean test1 test2 test3 all_tests output
